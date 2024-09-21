using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace ChatWebMvc.Misc
{
    public static class ControllerExtensions
    {
        public static OkObjectResult JsonOk(this ControllerBase controllerBase, object? data)
        {
            return controllerBase.Ok(new
            {
                code = 0,
                msg = "",
                data,
            });
        }

        public static ObjectResult JsonErr(this ControllerBase controllerBase, int code, string msg)
        {
            return controllerBase.StatusCode(500, new
            {
                code,
                msg,
            });
        }
    }
}
