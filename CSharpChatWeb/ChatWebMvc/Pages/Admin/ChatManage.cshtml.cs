using ChatWebMvc.Data;
using ChatWebMvc.Misc;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Pages.Admin
{
    [IgnoreAntiforgeryToken(Order = 1001)]
    public class ChatManageModel : PageModel
    {
        private readonly ApplicationDbContext _context;

        public ChatManageModel(ApplicationDbContext context)
        {
            _context = context;
        }

        public async Task<IActionResult> OnGetChatMsgsAsync(string keyword, int? msgId, int count)
        {
            if (count <= 0)
            {
                return Utils.JsonErr(-400, "invalid count");
            }

            var query = _context.ChatMsgs.IgnoreQueryFilters().Where(x => msgId == null || x.Id < msgId);
            if (!string.IsNullOrEmpty(keyword))
            {
                query = query.Where(x => x.Content.Contains(keyword));
            }

            var msgs = await query.OrderByDescending(x => x.Id).Take(count + 1).ToListAsync();
            var hasMore = msgs.Count > count;
            if (hasMore)
            {
                msgs = msgs[..^1];
            }

            return Utils.JsonOk(new
            {
                HasMore = hasMore,
                Msgs = msgs,
            });
        }

        public async Task<IActionResult> OnPostDeleteChatMsgAsync(int id)
        {
            var msg = await _context.ChatMsgs.FindAsync(id);
            if (msg == null)
            {
                return Utils.JsonErr(-404, "msg not found");
            }

            _context.ChatMsgs.Remove(msg);
            await _context.SaveChangesAsync();
            return Utils.JsonOk(null);
        }

        public async Task<IActionResult> OnPostUndeleteChatMsgAsync(int id)
        {
            var msg = await _context.ChatMsgs.IgnoreQueryFilters().FirstOrDefaultAsync(x => x.Id == id);
            if (msg == null)
            {
                return Utils.JsonErr(-404, "msg not found");
            }

            msg.IsDeleted = false;
            await _context.SaveChangesAsync();
            return Utils.JsonOk(null);
        }
    }
}
