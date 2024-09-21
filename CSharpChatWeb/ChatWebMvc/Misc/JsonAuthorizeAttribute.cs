using Microsoft.AspNetCore.Authorization;

namespace ChatWebMvc.Misc
{
    public class JsonAuthorizeAttribute : AuthorizeAttribute
    {
        public string Message { get; set; } = string.Empty;
    }
}
