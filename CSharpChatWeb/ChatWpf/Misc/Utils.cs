using ChatWpf.Models;
using ChatWpf.Models.Enums;
using Flurl;
using Flurl.Http;
using Flurl.Util;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace ChatWpf.Misc
{
    public static class Utils
    {
        public static readonly string MainWndToken = "MainWnd";
        public static readonly string HBase = "http://localhost:5291";
        public static readonly FlurlClient HClient = new FlurlClient
        {
            BaseUrl = HBase,
            Settings = {
                JsonSerializer = new Flurl.Http.Configuration.DefaultJsonSerializer(JsonOptions),
            },
        };
        public static readonly CookieJar HCookies = new CookieJar();

        public static readonly JsonSerializerOptions JsonOptions = new()
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            UnmappedMemberHandling = System.Text.Json.Serialization.JsonUnmappedMemberHandling.Skip,
        };

        public static async Task<JsonObject> HGetAsync(string url, IEnumerable<KeyValuePair<string, string>> parameters)
        {
            //return await HClient.GetFromJsonAsync<JsonObject>(url, parameters);
            //var response = await HClient.GetAsync(url + "?" + string.Join("&", parameters.Select(p => $"{p.Key}={p.Value}")));
            throw new NotImplementedException();
        }

        public static async Task<T> UnwrapJsonAsync<T>(this IFlurlResponse response)
        {
            var resp = await response.GetJsonAsync<ServerResponse<T>>();
            if (resp.Code < 0)
            {
                throw new Exception($"Request failed with code {resp.Code}: {resp.Msg}");
            }
            return resp.Data;
        }

        public static async Task UnwrapJsonAsync(this IFlurlResponse response)
        {
            var resp = await response.GetJsonAsync<ServerResponse>();
            if (resp.Code < 0)
            {
                throw new Exception($"Request failed with code {resp.Code}: {resp.Msg}");
            }
        }

        public static async Task<T> PostJsonAndUnwrapAsync<T>(this Flurl.Url url, object data)
        {
            return await (await url.AllowAnyHttpStatus().WithCookies(HCookies).PostJsonAsync(data)).UnwrapJsonAsync<T>();
        }

        public static async Task PostJsonAndUnwrapAsync(this Flurl.Url url, object data)
        {
            await (await url.AllowAnyHttpStatus().WithCookies(HCookies).PostJsonAsync(data)).UnwrapJsonAsync();
        }

        public static async Task<T> PostJsonAndUnwrapAsync<T>(this IFlurlRequest request, object data)
        {
            return await (await request.AllowAnyHttpStatus().WithCookies(HCookies).PostJsonAsync(data)).UnwrapJsonAsync<T>();
        }

        public static async Task PostJsonAndUnwrapAsync(this IFlurlRequest request, object data)
        {
            await (await request.AllowAnyHttpStatus().WithCookies(HCookies).PostJsonAsync(data)).UnwrapJsonAsync();
        }

        public static async Task<T> GetJsonAndUnwrapAsync<T>(this IFlurlRequest request)
        {
            return await (await request.AllowAnyHttpStatus().WithCookies(HCookies).GetAsync()).UnwrapJsonAsync<T>();
        }

        public static async Task GetJsonAndUnwrapAsync(this IFlurlRequest request)
        {
            await (await request.AllowAnyHttpStatus().WithCookies(HCookies).GetAsync()).UnwrapJsonAsync();
        }

        public static async Task<T> PostMultipartAndUnwrapAsync<T>(this IFlurlRequest request, Action<Flurl.Http.Content.CapturedMultipartContent> buildContent)
        {
            return await (await request.AllowAnyHttpStatus().WithCookies(HCookies).PostMultipartAsync(buildContent)).UnwrapJsonAsync<T>();
        }

        public static async Task PostMultipartAndUnwrapAsync(this IFlurlRequest request, Action<Flurl.Http.Content.CapturedMultipartContent> buildContent)
        {
            await (await request.AllowAnyHttpStatus().WithCookies(HCookies).PostMultipartAsync(buildContent)).UnwrapJsonAsync();
        }

        public static T? GetResource<T>(string key)
        {
            if (Application.Current.TryFindResource(key) is T res)
            {
                return res;
            }
            return default;
        }

        public static void GrowlError(string message)
        {
            HandyControl.Controls.Growl.Warning(new HandyControl.Data.GrowlInfo
            {
                Token = MainWndToken,
                Message = message,
                ShowDateTime = false,
                IsCustom = true,
                IconKey = "ErrorGeometry",
                IconBrushKey = "DangerBrush",
            });
        }

        public static void GrowlSuccess(string message)
        {
            HandyControl.Controls.Growl.Success(new HandyControl.Data.GrowlInfo
            {
                Token = MainWndToken,
                Message = message,
                ShowDateTime = false,
            });
        }

        public static void GrowlInfo(string message)
        {
            HandyControl.Controls.Growl.Info(new HandyControl.Data.GrowlInfo
            {
                Token = MainWndToken,
                Message = message,
                ShowDateTime = false,
            });
        }

        public static string FriendlyContent(this ChatMsg chatMsg)
        {
            return chatMsg.Kind switch
            {
                ChatMsgKind.PlainText => chatMsg.Content,
                ChatMsgKind.Image => chatMsg.Content[(chatMsg.Content.IndexOf('.') + 1)..],
                ChatMsgKind.File => chatMsg.Content[(chatMsg.Content.IndexOf('.') + 1)..],
                _ => "[未知消息]",
            };
        }

        public static async void ObserveUnhandledException<T>(this Task<T> task)
        {
            ArgumentNullException.ThrowIfNull(task, nameof(task));

            try
            {
                await task;
            }
            catch (Exception e)
            {
                Utils.GrowlError(e.Message);
            }
        }

        public static async void ObserveUnhandledException(this Task task)
        {
            ArgumentNullException.ThrowIfNull(task, nameof(task));

            try
            {
                await task;
            }
            catch (Exception e)
            {
                Utils.GrowlError(e.Message);
            }
        }

        public static Visual? GetDescendantByType(Visual? element, Type type)
        {
            if (element == null)
            {
                return null;
            }
            if (element.GetType() == type)
            {
                return element;
            }
            Visual? foundElement = null;
            if (element is FrameworkElement fe)
            {
                fe.ApplyTemplate();
            }
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(element); i++)
            {
                Visual? visual = VisualTreeHelper.GetChild(element, i) as Visual;
                foundElement = GetDescendantByType(visual, type);
                if (foundElement != null)
                {
                    break;
                }
            }
            return foundElement;
        }

        public static T? GetDescendant<T>(Visual? element) where T: class
        {
            if (element == null)
            {
                return default;
            }
            if (element is T t)
            {
                return t;
            }
            T? foundElement = null;
            if (element is FrameworkElement fe)
            {
                fe.ApplyTemplate();
            }
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(element); i++)
            {
                Visual? visual = VisualTreeHelper.GetChild(element, i) as Visual;
                foundElement = GetDescendant<T>(visual);
                if (foundElement != null)
                {
                    break;
                }
            }
            return foundElement as T;
        }

        public static bool IsImageFileName(string name)
        {
            var exts = new string[] { ".jpg", ".jpeg", ".png", ".bmp", ".gif" };
            name = name.ToLower();
            return exts.Any(ext => name.EndsWith(ext));
        }
    }

    public class ServerResponse<T>
    {
        public required int Code { get; set; }
        public required string Msg { get; set; }
        public required T Data { get; set; }
    }

    public class ServerResponse
    {
        public required int Code { get; set; }
        public required string Msg { get; set; }
        public object? Data { get; set; }
    }
}
