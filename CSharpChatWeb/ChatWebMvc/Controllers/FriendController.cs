using ChatWebMvc.Data;
using ChatWebMvc.Hubs;
using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;
using System.Diagnostics;
using System.Text.Json;

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonAuthorize]
    [JsonExceptionFilter]
    public class FriendController : ControllerBase
    {
        private readonly ApplicationDbContext _context;

        public FriendController(ApplicationDbContext context)
        {
            _context = context;
        }

        [HttpPost("request-add")]
        public async Task<IActionResult> RequestAddFriend([FromBody] (int targetId, string message) form)
        {
            var (targetId, message) = form;
            var userId = HttpContext.User.GetUserId();
            var friend = await _context.Users.FirstAsync(u => u.Id == targetId);

            var friendRequest = await _context.FriendRequests.FirstOrDefaultAsync(fr =>
                fr.FromId == userId && fr.ToId == targetId && fr.State == Models.Enums.FriendRequestState.Pending);
            if (friendRequest != null)
            {
                return this.JsonErr(-400, "已经发送好友请求，请等待对方确认");
            }

            if (await _context.UserFriendships.AnyAsync(uf =>
                (uf.FromId == userId && uf.ToId == targetId) || (uf.FromId == targetId && uf.ToId == userId)))
            {
                return this.JsonErr(-400, "已经是好友");
            }

            friendRequest = new FriendRequest
            {
                FromId = userId,
                ToId = targetId,
                RequestDate = DateTimeOffset.Now,
                RequestMessage = message,
                State = Models.Enums.FriendRequestState.Pending,
            };
            _context.FriendRequests.Add(friendRequest);
            await _context.SaveChangesAsync();
            return this.JsonOk(null);
        }

        [HttpPost("accept-add")]
        public async Task<IActionResult> AcceptAddFriend([FromBody] (int friendRequestId, object?) form)
        {
            var (friendRequestId, _) = form;
            var userId = HttpContext.User.GetUserId();
            var friendRequest = await _context.FriendRequests.FirstAsync(fr =>
                fr.Id == friendRequestId && fr.ToId == userId && fr.State == Models.Enums.FriendRequestState.Pending);
            friendRequest.State = Models.Enums.FriendRequestState.Accepted;
            _context.FriendRequests.Update(friendRequest);

            var curTime = DateTimeOffset.Now;
            _context.UserFriendships.Add(new UserFriendship
            {
                FromId = friendRequest.FromId,
                ToId = friendRequest.ToId,
                FriendshipDate = curTime,
            });
            _context.UserFriendships.Add(new UserFriendship
            {
                FromId = friendRequest.ToId,
                ToId = friendRequest.FromId,
                FriendshipDate = curTime,
            });

            // Initiate the chat by sending a message
            var chatMsg = new ChatMsg
            {
                SenderId = userId,
                ReceiverId = friendRequest.FromId,
                Content = "我们已经是好友了，开始聊天吧！",
                SendTime = curTime,
            };
            _context.ChatMsgs.Add(chatMsg);

            await _context.SaveChangesAsync();

            // Add friend to ChatHub's groups
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            foreach (var connectionId in connectionMapping.GetConnections(friendRequest.FromId))
            {
                await hub.Groups.AddToGroupAsync(connectionId, userId.ToString());
            }
            foreach (var connectionId in connectionMapping.GetConnections(userId))
            {
                await hub.Groups.AddToGroupAsync(connectionId, friendRequest.FromId.ToString());
            }
            // Message notify: sender & receiver
            await hub.Clients.Users(userId.ToString(), friendRequest.FromId.ToString())
                .SendAsync("OnReceiveNewMessage", userId, friendRequest.FromId, chatMsg.Id,
                Utils.SerializeToCamelCaseJson(chatMsg));

            return this.JsonOk(null);
        }

        [HttpPost("reject-add")]
        public async Task<IActionResult> RejectAddFriend([FromBody] (int friendRequestId, object?) form)
        {
            var (friendRequestId, _) = form;
            var userId = HttpContext.User.GetUserId();
            var friendRequest = await _context.FriendRequests.FirstAsync(fr =>
                fr.Id == friendRequestId && fr.ToId == userId && fr.State == Models.Enums.FriendRequestState.Pending);
            friendRequest.State = Models.Enums.FriendRequestState.Rejected;
            _context.FriendRequests.Update(friendRequest);
            await _context.SaveChangesAsync();

            return this.JsonOk(null);
        }

        [HttpPost("remove-friend")]
        public async Task<IActionResult> RemoveFriend([FromBody] (int friendId, object?) form)
        {
            var (friendId, _) = form;
            var userId = HttpContext.User.GetUserId();
            var friendship = await _context.UserFriendships.Where(uf =>
                (uf.FromId == userId && uf.ToId == friendId) || (uf.FromId == friendId && uf.ToId == userId)).ToListAsync();
            if (friendship.Count == 0)
            {
                return this.JsonErr(-400, "不是好友");
            }
            _context.UserFriendships.RemoveRange(friendship);
            await _context.SaveChangesAsync();

            // Remove friend from ChatHub's groups
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            foreach (var connectionId in connectionMapping.GetConnections(friendId))
            {
                await hub.Groups.RemoveFromGroupAsync(connectionId, userId.ToString());
            }
            foreach (var connectionId in connectionMapping.GetConnections(userId))
            {
                await hub.Groups.RemoveFromGroupAsync(connectionId, friendId.ToString());
            }
            // Notify deletion
            await hub.Clients.Users(userId.ToString(), friendId.ToString())
                .SendAsync("OnReceiveNewMessage", userId, friendId, 0,
                Utils.SerializeToCamelCaseJson(new { Kind = "friend-removed", FriendA = userId, FriendB = friendId }));

            return this.JsonOk(null);
        }

        [HttpGet("my-friend-requests")]
        public async Task<IActionResult> GetMyFriendRequests()
        {
            var userId = HttpContext.User.GetUserId();
            var friendRequests = await _context.FriendRequests
                .Where(fr => fr.ToId == userId)
                .Select(fr => new { fr.Id, From = fr.From.ToSlim(), fr.RequestMessage, fr.State })
                .OrderByDescending(fr => fr.Id)
                .ToListAsync();
            return this.JsonOk(friendRequests);
        }

        [HttpGet("my-friends")]
        public async Task<IActionResult> GetMyFriends()
        {
            var userId = HttpContext.User.GetUserId();
            var friends = await _context.UserFriendships
                .Where(uf => uf.FromId == userId)
                .Select(uf => uf.To.ToSlim())
                .ToListAsync();
            return this.JsonOk(friends);
        }
    }
}
