using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Authorization.Policy;
using Microsoft.AspNetCore.Http;

namespace ChatWebMvc.Misc
{
    public class MyAuthorizationMiddlewareResultHandler : IAuthorizationMiddlewareResultHandler
    {
        private async Task<bool> DoWork(HttpContext context)
        {
            var endpoint = context.GetEndpoint();
            var jsonHeader = endpoint?.Metadata?.GetMetadata<JsonAuthorizeAttribute>();
            if (jsonHeader != null)
            {
                var message = "用户凭证无效";

                if (!string.IsNullOrEmpty(jsonHeader.Message))
                {
                    message = jsonHeader.Message;
                }

                context.Response.StatusCode = 401;
                context.Response.ContentType = "application/json";
                var jsonResponse = System.Text.Json.JsonSerializer.Serialize(new
                {
                    code = -401,
                    msg = message,
                });

                await context.Response.WriteAsync(jsonResponse);
                return true;
            }

            return false;
        }

        public async Task HandleAsync(RequestDelegate next, HttpContext context, AuthorizationPolicy policy, PolicyAuthorizationResult authorizeResult)
        {
            if (authorizeResult.Challenged)
            {
                if (await DoWork(context))
                {
                    return;
                }

                if (policy.AuthenticationSchemes.Count > 0)
                {
                    foreach (var scheme in policy.AuthenticationSchemes)
                    {
                        await context.ChallengeAsync(scheme);
                    }
                }
                else
                {
                    await context.ChallengeAsync();
                }

                return;
            }
            else if (authorizeResult.Forbidden)
            {
                if (await DoWork(context))
                {
                    return;
                }

                if (policy.AuthenticationSchemes.Count > 0)
                {
                    foreach (var scheme in policy.AuthenticationSchemes)
                    {
                        await context.ForbidAsync(scheme);
                    }
                }
                else
                {
                    await context.ForbidAsync();
                }

                return;
            }

            await next(context);
        }
    }
}
