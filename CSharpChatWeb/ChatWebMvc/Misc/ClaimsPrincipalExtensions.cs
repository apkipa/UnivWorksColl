using System.Security.Claims;

namespace ChatWebMvc.Misc
{
    public static class ClaimsPrincipalExtensions
    {
        public static int? GetUserIdOrDefault(this System.Security.Claims.ClaimsPrincipal user)
        {
            var userId = user.FindFirstValue(ClaimTypes.NameIdentifier);
            if (userId == null)
            {
                return null;
            }
            return int.Parse(userId);
        }
        public static int GetUserId(this System.Security.Claims.ClaimsPrincipal user)
        {
            return user.GetUserIdOrDefault() ?? throw new Exception("用户不存在");
        }
    }
}
