using ChatWebMvc.Models;
using Microsoft.AspNetCore.Identity;

namespace ChatWebMvc.Misc
{
    public class MyUserValidator : IUserValidator<AppUser>
    {
        public Task<IdentityResult> ValidateAsync(UserManager<AppUser> manager, AppUser user)
        {
            if (user.IsBanned)
            {
                return Task.FromResult(IdentityResult.Failed(new IdentityError { Code = "Banned", Description = "用户已被封禁。" }));
            }
            else
            {
                return Task.FromResult(IdentityResult.Success);
            }
        }
    }
}
