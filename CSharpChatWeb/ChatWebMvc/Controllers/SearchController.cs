using ChatWebMvc.Data;
using ChatWebMvc.Misc;
using ChatWebMvc.Models.Enums;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonAuthorize]
    [JsonExceptionFilter]
    public class SearchController : ControllerBase
    {
        private readonly ApplicationDbContext _context;

        public SearchController(ApplicationDbContext context)
        {
            _context = context;
        }

        [HttpGet("search-principals")]
        public async Task<IActionResult> SearchPrincipals([FromQuery] string keyword)
        {
            if (string.IsNullOrEmpty(keyword))
            {
                return this.JsonErr(-400, "请输入关键字");
            }

            var userId = User.GetUserId();
            var users = await _context.Users
                .Where(u => u.Id.ToString().Contains(keyword))
                .Select(u => u.ToSlim())
                .ToListAsync();
            var groups = await _context.ChatGroups
                .Where(g => g.Id.ToString().Contains(keyword))
                .Select(g => new { g.Id, g.Name })
                .ToListAsync();
            return this.JsonOk(new { users, groups });
        }

        [HttpGet("search-msgs")]
        public async Task<IActionResult> SearchMsgs([FromQuery] int? principalId, [FromQuery] string keyword, [FromQuery] int? msgId, [FromQuery] int msgCount)
        {
            if (msgCount <= 0)
            {
                return this.JsonErr(-400, "msgCount 必须大于 0");
            }

            var includeCount = msgId == null;

            var userId = User.GetUserId();

            IQueryable<ChatMsg> query = _context.ChatMsgs.Include(x => x.Receiver);
            if (principalId == null)
            {
                // Find in all chats
                /*query = query.Where(
                    x => x.SenderId == userId || x.ReceiverId == userId ||
                    _context.UserGroupJoins.Where(x => x.UserId == userId).Select(x => x.GroupId).Contains(x.ReceiverId));*/
                query = query.Where(x => _context.UserFriendships.Any(uf => (uf.FromId == userId || uf.ToId == userId) && (x.SenderId == uf.FromId && x.ReceiverId == uf.ToId)) ||
                    _context.UserGroupJoins.Any(ug => ug.UserId == userId && ug.GroupId == x.ReceiverId));
            }
            else
            {
                // Find in a specific chat

                // Verify relationship
                var principal = await _context.PrincipalNumbers.FirstAsync(x => x.Id == principalId);
                if (principal.Kind == PrincipalKind.User)
                {
                    if (!await _context.UserFriendships.AnyAsync(x => x.FromId == userId && x.ToId == principalId || x.FromId == principalId && x.ToId == userId))
                    {
                        return this.JsonErr(-400, "不是好友");
                    }
                    query = query.Where(x => x.SenderId == principalId && x.ReceiverId == userId || x.SenderId == userId && x.ReceiverId == principalId);
                }
                else if (principal.Kind == PrincipalKind.Group)
                {
                    if (!await _context.UserGroupJoins.AnyAsync(x => x.UserId == userId && x.GroupId == principalId))
                    {
                        return this.JsonErr(-400, "不是群组成员");
                    }
                    query = query.Where(x => x.ReceiverId == principalId);
                }
                else
                {
                    return this.JsonErr(-400, "未知的 principalId");
                }
            }

            if (!string.IsNullOrEmpty(keyword))
            {
                query = query.Where(x => (x.Kind == ChatMsgKind.File ? x.Content.Substring(64 + 1) : x.Content).Contains(keyword));
            }
            if (msgId != null)
            {
                query = query.Where(x => x.Id < msgId);
            }

            int? totalMsgCount = includeCount ? await query.CountAsync() : null;

            Func<int, PrincipalKind, object?> getDetails = (id, kind) => kind switch
            {
                PrincipalKind.User => _context.Users.Where(u => u.Id == id).Select(u => u.ToSlim()).FirstOrDefault(),
                PrincipalKind.Group => _context.ChatGroups.Where(g => g.Id == id).Select(g => g).FirstOrDefault(),
                _ => null,
            };
            var msgs = await query.OrderByDescending(x => x.Id).Take(msgCount + 1)
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
                    ReceiverDetails = x.Receiver != null ? x.Receiver.Kind == PrincipalKind.User ? _context.Users.Where(u => u.Id == x.ReceiverId).Select(u => u.ToSlim()).FirstOrDefault() : _context.ChatGroups.Where(g => g.Id == x.ReceiverId).Select(g => g).FirstOrDefault() : null,
                })
                .ToListAsync();

            var hasMore = msgs.Count > msgCount;
            if (hasMore)
            {
                msgs = msgs[..^1];
            }

            return this.JsonOk(new
            {
                Messages = msgs,
                HasMore = hasMore,
                TotalCount = totalMsgCount,
            });
        }
    }
}
