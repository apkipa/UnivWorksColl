using ChatWebMvc.Data;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Pages.Admin
{
    public class AccountManageModel : PageModel
    {
        private readonly ApplicationDbContext _context;
        private readonly UserManager<AppUser> _userManager;

        public AccountManageModel(ApplicationDbContext context, UserManager<AppUser> userManager)
        {
            _context = context;
            _userManager = userManager;
        }

        public List<AppUser> Users { get; set; } = null!;

        private async Task PopulateFieldsAsync()
        {
            Users = await _context.Users.ToListAsync();
        }

        public async Task OnGetAsync()
        {
            await PopulateFieldsAsync();
        }

        public async Task OnPostBanAsync(int id)
        {
            var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == id);
            if (user != null)
            {
                if (user.UserName == "admin")
                {
                    TempData["MessageColor"] = "red";
                    TempData["Message"] = "FAILSAFE: ���������ڽ�����Ա�˻���";
                }
                else
                {
                    user.IsBanned = true;
                    await _context.SaveChangesAsync();

                    // Kick user sessions (sign out everywhere)
                    await _userManager.UpdateSecurityStampAsync(user);

                    TempData["Message"] = "�ѳɹ�����û���";
                }
            }

            await PopulateFieldsAsync();
        }

        public async Task OnPostPardonAsync(int id)
        {
            var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == id);
            if (user != null)
            {
                user.IsBanned = false;
                await _context.SaveChangesAsync();

                TempData["Message"] = "�ѳɹ�����û���";
            }

            await PopulateFieldsAsync();
        }
    }
}
