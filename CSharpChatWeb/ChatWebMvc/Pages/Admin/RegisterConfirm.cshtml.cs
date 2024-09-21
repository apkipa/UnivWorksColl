using ChatWebMvc.Data;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Pages.Admin
{
    public class RegisterConfirmModel : PageModel
    {
        private readonly ApplicationDbContext _context;

        public RegisterConfirmModel(ApplicationDbContext context)
        {
            _context = context;
        }

        public List<AppUser> Users { get; set; } = null!;

        private async Task PopulateFieldsAsync()
        {
            Users = await _context.Users.Where(u => u.EmailConfirmed == false).ToListAsync();
        }

        public async Task OnGetAsync()
        {
            await PopulateFieldsAsync();
        }

        public async Task OnPostAcceptAsync(int id)
        {
            var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == id);
            if (user != null)
            {
                user.EmailConfirmed = true;
                await _context.SaveChangesAsync();

                TempData["Message"] = "已成功通过用户注册。";
            }

            await PopulateFieldsAsync();
        }

        public async Task OnPostRejectAsync(int id)
        {
            var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == id);
            if (user != null)
            {
                _context.Users.Remove(user);
                await _context.SaveChangesAsync();

                TempData["Message"] = "已成功拒绝用户注册。";
            }

            await PopulateFieldsAsync();
        }
    }
}
