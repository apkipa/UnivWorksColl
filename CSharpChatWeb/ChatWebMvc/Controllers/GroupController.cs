using ChatWebMvc.Data;
using ChatWebMvc.Hubs;
using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using ChatWebMvc.Models.Enums;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonAuthorize]
    [JsonExceptionFilter]
    public class GroupController : ControllerBase
    {
        private readonly ApplicationDbContext _context;

        public GroupController(ApplicationDbContext context)
        {
            _context = context;
        }

        [HttpPost("create")]
        public async Task<IActionResult> CreateGroup([FromBody] (string groupName, object?) form)
        {
            var (groupName, _) = form;
            var userId = User.GetUserId();
            var curTime = DateTimeOffset.Now;
            var group = new ChatGroup
            {
                PrincipalNumber = new PrincipalNumber
                {
                    Kind = PrincipalKind.Group
                },
                Name = groupName,
                CreationDate = curTime,
                OwnerId = userId,
            };
            _context.ChatGroups.Add(group);
            group.UserGroupJoins.Add(new UserGroupJoin
            {
                UserId = userId,
                GroupId = group.Id,
                JoinDate = curTime,
                IsOperator = true,
            });

            // Initiate the chat by sending a message
            var msg = new ChatMsg
            {
                SenderId = userId,
                ReceiverId = _context.Entry(group).Property(e => e.Id).CurrentValue,
                SendTime = curTime,
                Content = $"群组已经创建，快来聊天吧！",
            };
            _context.ChatMsgs.Add(msg);

            await _context.SaveChangesAsync();

            // Add user to ChatHub's group
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            foreach (var connectionId in connectionMapping.GetConnections(userId))
            {
                await hub.Groups.AddToGroupAsync(connectionId, group.Id.ToString());
            }
            // Notify
            await hub.Clients.Groups(group.Id.ToString()).SendAsync("OnReceiveNewMessage", userId, group.Id, msg.Id, Utils.SerializeToCamelCaseJson(msg));

            return this.JsonOk(group);
        }

        [HttpPost("delete")]
        public async Task<IActionResult> DeleteGroup([FromBody] (int groupId, object?) form)
        {
            var (groupId, _) = form;
            var userId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == userId && x.IsOperator)) {
                return this.JsonErr(-400, "不能删除没有管理权限的群组");
            }

            group.UserGroupJoins.Clear();
            _context.ChatGroups.Remove(group);

            // Remove messages in group
            //_context.ChatMsgs.RemoveRange(_context.ChatMsgs.Where(x => x.ReceiverId == groupId));

            await _context.SaveChangesAsync();

            // Remove all users from ChatHub's group
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            // Notify
            await hub.Clients.Group(groupId.ToString()).SendAsync("OnReceiveNewMessage", userId, groupId, 0,
                Utils.SerializeToCamelCaseJson(new { Kind = "group-removed", GroupId = groupId }));
            // Remove
            foreach (var userGroupJoin in group.UserGroupJoins)
            {
                foreach (var connectionId in connectionMapping.GetConnections(userGroupJoin.UserId))
                {
                    await hub.Groups.RemoveFromGroupAsync(connectionId, groupId.ToString());
                }
            }

            return this.JsonOk(null);
        }

        [HttpGet("list-members")]
        public async Task<IActionResult> ListMembers([FromQuery] int groupId)
        {
            var userId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "不是群成员");
            }

            var members = await _context.UserGroupJoins
                .Where(x => x.GroupId == groupId)
                .Select(x => new { x.User.Id, x.User.UserName, x.IsOperator })
                .ToListAsync();
            return this.JsonOk(members);
        }

        [HttpGet("list-operators")]
        public async Task<IActionResult> ListOperators([FromQuery] int groupId)
        {
            var userId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "不是群成员");
            }

            var operators = await _context.UserGroupJoins
                .Where(x => x.GroupId == groupId && x.IsOperator)
                .Select(x => new { x.User.Id, x.User.UserName })
                .ToListAsync();
            return this.JsonOk(operators);
        }

        [HttpPost("add-member")]
        public async Task<IActionResult> AddMember([FromBody] (int groupId, int userId) form)
        {
            var (groupId, userId) = form;
            var curUserId = User.GetUserId();
            var senderDetails = await _context.Users.Where(x => x.Id == userId).Select(x => x.ToSlim()).FirstAsync();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == curUserId && x.IsOperator)) {
                return this.JsonErr(-400, "没有权限添加成员");
            }
            if (group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "已经是群成员");
            }

            if (!_context.UserFriendships.Any(x => x.FromId == userId && x.ToId == curUserId))
            {
                return this.JsonErr(-400, "只能邀请好友加入群组");
            }

            var userName = await _context.Users.Where(x => x.Id == userId).Select(x => x.UserName).FirstAsync();

            group.UserGroupJoins.Add(new UserGroupJoin
            {
                UserId = userId,
                GroupId = groupId,
                JoinDate = DateTimeOffset.Now,
            });

            var chatMsg = new ChatMsg
            {
                SenderId = userId,
                ReceiverId = groupId,
                Content = $"大家好，我是{userName}。",
                SendTime = DateTimeOffset.Now,
            };
            _context.ChatMsgs.Add(chatMsg);

            await _context.SaveChangesAsync();

            // Add user to ChatHub's group
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            foreach (var connectionId in connectionMapping.GetConnections(userId))
            {
                await hub.Groups.AddToGroupAsync(connectionId, groupId.ToString());
            }
            // Notify
            await hub.Clients.Group(groupId.ToString()).SendAsync("OnReceiveNewMessage", userId, groupId, chatMsg.Id,
                Utils.SerializeToCamelCaseJson(chatMsg.WithDetails(senderDetails)));

            return this.JsonOk(null);
        }

        [HttpPost("remove-member")]
        public async Task<IActionResult> RemoveMember([FromBody] (int groupId, int userId) form)
        {
            var (groupId, userId) = form;
            var curUserId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (curUserId != userId && !group.UserGroupJoins.Any(x => x.UserId == curUserId && x.IsOperator)) {
                return this.JsonErr(-400, "没有权限删除成员");
            }
            if (!group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "不是群成员");
            }
            if (group.OwnerId == userId) {
                return this.JsonErr(-400, "不能删除群主");
            }

            var userGroupJoin = group.UserGroupJoins.First(x => x.UserId == userId);
            group.UserGroupJoins.Remove(userGroupJoin);
            await _context.SaveChangesAsync();

            // Remove user from ChatHub's group
            var hub = HttpContext.RequestServices.GetRequiredService<IHubContext<ChatHub>>();
            // Notify
            await hub.Clients.User(userId.ToString()).SendAsync("OnReceiveNewMessage", userId, groupId, 0,
                Utils.SerializeToCamelCaseJson(new { Kind = "group-removed", GroupId = groupId }));
            // Remove
            var connectionMapping = HttpContext.RequestServices.GetRequiredService<IConnectionMapping<int>>();
            foreach (var connectionId in connectionMapping.GetConnections(userId))
            {
                await hub.Groups.RemoveFromGroupAsync(connectionId, groupId.ToString());
            }

            return this.JsonOk(null);
        }

        [HttpPost("set-operator")]
        public async Task<IActionResult> SetOperator([FromBody] (int groupId, int userId) form)
        {
            var (groupId, userId) = form;
            var curUserId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == curUserId && x.IsOperator)) {
                return this.JsonErr(-400, "没有权限设置管理员");
            }
            if (!group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "不是群成员");
            }

            var userGroupJoin = group.UserGroupJoins.First(x => x.UserId == userId);
            userGroupJoin.IsOperator = true;
            await _context.SaveChangesAsync();
            return this.JsonOk(null);
        }

        [HttpPost("unset-operator")]
        public async Task<IActionResult> UnsetOperator([FromBody] (int groupId, int userId) form)
        {
            var (groupId, userId) = form;
            var curUserId = User.GetUserId();
            var group = await _context.ChatGroups.Include(x => x.UserGroupJoins).FirstAsync(g => g.Id == groupId);
            if (!group.UserGroupJoins.Any(x => x.UserId == curUserId && x.IsOperator)) {
                return this.JsonErr(-400, "没有权限取消管理员");
            }
            if (!group.UserGroupJoins.Any(x => x.UserId == userId)) {
                return this.JsonErr(-400, "不是群成员");
            }
            if (userId == curUserId) {
                return this.JsonErr(-400, "不能取消自己的管理员权限");
            }
            if (group.OwnerId == userId) {
                return this.JsonErr(-400, "不能取消群主的管理员权限");
            }

            var userGroupJoin = group.UserGroupJoins.First(x => x.UserId == userId);
            userGroupJoin.IsOperator = false;
            await _context.SaveChangesAsync();
            return this.JsonOk(null);
        }

        [HttpGet("my-groups")]
        public async Task<IActionResult> GetMyGroups()
        {
            var userId = User.GetUserId();
            var groups = await _context.Users
                .Include(x => x.UserGroupJoins)
                .FirstAsync(x => x.Id == userId);
            return this.JsonOk(groups.UserGroupJoins.Select(x => x.Group));
        }
    }
}
