using Microsoft.AspNetCore.Mvc.Filters;
using Microsoft.AspNetCore.Mvc;

namespace ChatWebMvc.Misc
{
    public class JsonExceptionFilter : IExceptionFilter
    {
        public void OnException(ExceptionContext context)
        {
            var result = new ObjectResult(new
            {
                code = -1,
                msg = $"发生了服务器内部错误: {context.Exception.Message}",
            });

            result.StatusCode = 500;
            context.Result = result;
        }
    }
}
