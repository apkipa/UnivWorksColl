using ChatWebMvc.Data;
using ChatWebMvc.Hubs;
using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using ChatWebMvc.Models.Enums;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonAuthorize]
    [JsonExceptionFilter]
    public class ChatController : ControllerBase
    {
        private readonly ApplicationDbContext _context;

        public ChatController(ApplicationDbContext context)
        {
            _context = context;
        }

        [HttpGet("recent-chats")]
        public async Task<IActionResult> GetRecentChats()
        {
            var userId = User.GetUserId();

            var friendChats = _context.UserFriendships
                .Where(x => x.FromId == userId)
                .Select(x => x.To)
                .SelectMany(friend => _context.ChatMsgs
                    .Where(chat => chat.SenderId == friend.Id && chat.ReceiverId == userId || chat.SenderId == userId && chat.ReceiverId == friend.Id)
                    .OrderByDescending(chat => chat.Id)
                    .Take(1)
                    .DefaultIfEmpty()
                    .Select(chat => new { Target = friend.ToSlim(), Chat = chat }));
            var groupChats = _context.UserGroupJoins
                .Where(x => x.UserId == userId)
                .Select(x => x.Group)
                .Select(g => new { Target = g, Chat = _context.ChatMsgs
                    .Where(chat => chat.ReceiverId == g.Id)
                    .OrderByDescending(chat => chat.Id)
                    .Select(x => new
                    {
                        x.Id,
                        x.SenderId,
                        x.Sender,
                        x.ReceiverId,
                        x.Receiver,
                        x.Kind,
                        x.Content,
                        x.SendTime,
                        SenderDetails = x.Sender != null ? x.Sender.Kind == PrincipalKind.User ? _context.Users.Where(u => u.Id == x.SenderId).Select(u => u.ToSlim()).FirstOrDefault() : _context.ChatGroups.Where(g => g.Id == x.SenderId).Select(g => g).FirstOrDefault() : null,
                        ReceiverDetails = (object?)null,
                    })
                    .FirstOrDefault()
                });
                /*.SelectMany(g => _context.ChatMsgs
                    .Where(chat => chat.ReceiverId == g.Id)
                    .OrderByDescending(chat => chat.Id)
                    .Select(x => new
                    {
                        x.Id,
                        x.SenderId,
                        x.Sender,
                        x.ReceiverId,
                        x.Receiver,
                        x.Kind,
                        x.Content,
                        x.SendTime,
                        SenderDetails = x.Sender != null ? x.Sender.Kind == PrincipalKind.User ? _context.Users.Where(u => u.Id == x.SenderId).Select(u => u.ToSlim()).FirstOrDefault() : _context.ChatGroups.Where(g => g.Id == x.SenderId).Select(g => g).FirstOrDefault() : null,
                        ReceiverDetails = (object?)null,
                    })
                    .Take(1)
                    .DefaultIfEmpty()
                    .Select(x => new { Target = g, Chat = x }));*/

            return this.JsonOk(new
            {
                Users = await friendChats.ToListAsync(),
                Groups = await groupChats.ToListAsync(),
            });
        }

        [HttpGet("get-msgs")]
        public async Task<IActionResult> GetMsgs([FromQuery] int principalId, [FromQuery] int? msgId, [FromQuery] string direction, [FromQuery] int msgCount)
        {
            if (msgCount <= 0) {
                return this.JsonErr(-400, "msgCount 必须大于 0");
            }

            var userId = User.GetUserId();
            var principal = await _context.PrincipalNumbers.FirstAsync(x => x.Id == principalId);
            bool isGroup = false;
            if (principal.Kind == PrincipalKind.User)
            {
                if (!await _context.UserFriendships.AnyAsync(x => x.FromId == userId && x.ToId == principalId || x.FromId == principalId && x.ToId == userId))
                {
                    return this.JsonErr(-400, "不是好友");
                }
            }
            else if (principal.Kind == PrincipalKind.Group)
            {
                isGroup = true;
                if (!await _context.UserGroupJoins.AnyAsync(x => x.UserId == userId && x.GroupId == principalId))
                {
                    return this.JsonErr(-400, "不是群组成员");
                }
            }
            else
            {
                return this.JsonErr(-400, "未知的 principalId");
            }

            var msgsQuery = _context.ChatMsgs
                .Where(x => isGroup ? x.ReceiverId == principalId : (x.SenderId == principalId && x.ReceiverId == userId || x.SenderId == userId && x.ReceiverId == principalId))
                .Where(x => msgId == null || (direction == "before" ? x.Id <= msgId : x.Id >= msgId));

            switch (direction)
            {
                case "before":
                    msgsQuery = msgsQuery.OrderByDescending(x => x.Id).Take(msgCount + 1);
                    break;
                case "after":
                    msgsQuery = msgsQuery.OrderBy(x => x.Id).Take(msgCount + 1);
                    break;
                default:
                    return this.JsonErr(-400, "未知的 direction");
            }
            var msgs = await msgsQuery
                .Select(x => new
                {
                    x.Id,
                    x.SenderId,
                    x.Sender,
                    x.ReceiverId,
                    x.Receiver,
                    x.Kind,
                    x.Content,
                    x.SendTime,
                    SenderDetails = x.Sender != null ? x.Sender.Kind == PrincipalKind.User ? _context.Users.Where(u => u.Id == x.SenderId).Select(u => u.ToSlim()).FirstOrDefault() : _context.ChatGroups.Where(g => g.Id == x.SenderId).Select(g => g).FirstOrDefault() : null,
                    ReceiverDetails = (object?)null,
                })
                .ToListAsync();
            bool hasMore = msgs.Count > msgCount;
            if (hasMore)
            {
                msgs = msgs[..^1];
            }
            if (direction == "before")
            {
                msgs.Reverse();
            }

            return this.JsonOk(new
            {
                Messages = msgs,
                HasMore = hasMore
            });

#if false
            List<ChatMsg> msgs;
            bool hasMore;
            switch (direction)
            {
                case "before":
                    msgs = await msgsQuery.OrderByDescending(x => x.Id).Take(msgCount + 1).ToListAsync();
                    hasMore = msgs.Count > msgCount;
                    if (hasMore)
                    {
                        msgs = msgs[..^1];
                    }
                    msgs.Reverse();
                    break;
                case "after":
                    msgs = await msgsQuery.OrderBy(x => x.Id).Take(msgCount + 1).ToListAsync();
                    hasMore = msgs.Count > msgCount;
                    if (hasMore)
                    {
                        msgs = msgs[..^1];
                    }
                    break;
                default:
                    return this.JsonErr(-400, "未知的 direction");
            }

            /*var totalMsgCount = await msgsQuery.CountAsync();
            var msgs = direction == "before" ?
                await msgsQuery.AsAsyncEnumerable().TakeLast(msgCount).ToListAsync() :
                await msgsQuery.Take(msgCount).ToListAsync();*/
            //var hasMore = totalMsgCount > msgCount;

            return this.JsonOk(new
            {
                Messages = msgs,
                HasMore = hasMore
            });
#endif
        }

        [HttpPost("send-msg")]
        public async Task<IActionResult> SendMsg([FromBody] (int principalId, string content, string type) form)
        {
            var (principalId, content, type) = form;
            var userId = User.GetUserId();
            var senderDetails = (await _context.Users.FirstAsync(x => x.Id == userId)).ToSlim();
            var receiver = await _context.PrincipalNumbers.FirstAsync(x => x.Id == principalId);
            if (receiver.Kind == PrincipalKind.User)
            {
                if (!await _context.UserFriendships.AnyAsync(x => x.FromId == userId && x.ToId == principalId || x.FromId == principalId && x.ToId == userId))
                {
                    return this.JsonErr(-400, "不是好友");
                }
            }
            else if (receiver.Kind == PrincipalKind.Group)
            {
                if (!await _context.UserGroupJoins.AnyAsync(x => x.UserId == userId && x.GroupId == principalId))
                {
                    return this.JsonErr(-400, "不是群组成员");
                }
            }
            else
            {
                return this.JsonErr(-400, "未知的 principalId");
            }

            var msgKind = type switch
            {
                "PlainText" => ChatMsgKind.PlainText,
                "Image" => ChatMsgKind.Image,
                "File" => ChatMsgKind.File,
                "RichXml" => ChatMsgKind.RichXml,
                _ => ChatMsgKind.Unknown,
            };
            if (msgKind == ChatMsgKind.Unknown)
            {
                return this.JsonErr(-400, "未知的消息类型");
            }

            var msg = new ChatMsg
            {
                SenderId = userId,
                ReceiverId = principalId,
                Kind = msgKind,
                Content = content,
                SendTime = DateTimeOffset.Now,
            };
            _context.ChatMsgs.Add(msg);
            await _context.SaveChangesAsync();

            // Broadcast the new message to both sender and receiver
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            /*await hub.Clients.Groups(principalId.ToString(), userId.ToString()).SendAsync("OnReceiveNewMessage", userId, principalId, msg.Id,
                Utils.SerializeToCamelCaseJson(msg));*/
            await hub.Clients.Group(principalId.ToString()).SendAsync("OnReceiveNewMessage", userId, principalId, msg.Id,
                Utils.SerializeToCamelCaseJson(msg.WithDetails(senderDetails)));

            return this.JsonOk(null);
        }

        [HttpPost("delete-msg")]
        public async Task<IActionResult> DeleteMsg([FromBody] (int msgId, object?) form)
        {
            var (msgId, _) = form;
            var userId = User.GetUserId();
            var msg = await _context.ChatMsgs.FirstOrDefaultAsync(x => x.Id == msgId);
            if (msg == null)
            {
                return this.JsonErr(-400, "消息不存在或已删除");
            }
            if (msg.SenderId != userId)
            {
                return this.JsonErr(-400, "只能删除自己发送的消息");
            }

            _context.ChatMsgs.Remove(msg);
            await _context.SaveChangesAsync();

            // Broadcast the deletion to both sender and receiver
            var principalId = msg.ReceiverId;
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            await hub.Clients.Groups(userId.ToString(), principalId.ToString()).SendAsync("OnReceiveNewMessage", userId, principalId, msgId,
                Utils.SerializeToCamelCaseJson(msg));

            return this.JsonOk(null);
        }
    }
}
