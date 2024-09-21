using Microsoft.AspNetCore.Mvc;
using System.Text.Json;

namespace ChatWebMvc.Misc
{
    public static class Utils
    {
        public static JsonSerializerOptions CamelCaseJsonOptions => new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        };

        public static string SerializeToCamelCaseJson(object obj)
        {
            return JsonSerializer.Serialize(obj, CamelCaseJsonOptions);
        }

        public static async IAsyncEnumerable<R> SelectManyAsync<T, R>(this IEnumerable<T> ts, Func<T, Task<IEnumerable<R>>> func)
        {
            foreach (var t in ts)
            {
                var rs = await func(t);
                foreach (var r in rs)
                    yield return r;
            }
        }

        public static async IAsyncEnumerable<R> SelectManyAsync<T, R>(this IEnumerable<T> ts, Func<T, IAsyncEnumerable<R>> func)
        {
            foreach (var t in ts)
            {
                var rs = func(t);
                await foreach (var r in rs)
                    yield return r;
            }
        }

        public static IActionResult JsonOk(object? data)
        {
            return new JsonResult(new
            {
                code = 0,
                msg = "",
                data,
            });
        }

        public static IActionResult JsonErr(int code, string msg)
        {
            return new JsonResult(new
            {
                code,
                msg,
            });
        }

        public static object ToSlim(this Models.AppUser user)
        {
            return new
            {
                user.Id,
                user.UserName,
                user.Nickname,
            };
        }

        public static object WithDetails(this Models.ChatMsg chatMsg, object? senderDetails, object? receiverDetails = null)
        {
            return new
            {
                chatMsg.Id,
                chatMsg.SenderId,
                chatMsg.Sender,
                chatMsg.ReceiverId,
                chatMsg.Receiver,
                chatMsg.Kind,
                chatMsg.Content,
                chatMsg.SendTime,
                SenderDetails = senderDetails,
                ReceiverDetails = receiverDetails,
            };
        }
    }

    public interface IConnectionMapping<T> where T : notnull
    {
        public int Count { get; }

        public void Add(T key, string connectionId);

        public IEnumerable<string> GetConnections(T key);

        public void Remove(T key, string connectionId);
    }

    public class ConnectionMapping<T> : IConnectionMapping<T> where T : notnull
    {
        private readonly Dictionary<T, HashSet<string>> _connections = [];

        public int Count => _connections.Count;

        public void Add(T key, string connectionId)
        {
            lock (_connections)
            {
                if (!_connections.TryGetValue(key, out var connections))
                {
                    connections = [];
                    _connections[key] = connections;
                }
                lock (connections)
                {
                    connections.Add(connectionId);
                }
            }
        }

        public IEnumerable<string> GetConnections(T key)
        {
            if (_connections.TryGetValue(key, out var connections))
            {
                return connections;
            }
            return [];
        }

        public void Remove(T key, string connectionId)
        {
            lock (_connections)
            {
                if (_connections.TryGetValue(key, out var connections))
                {
                    lock (connections)
                    {
                        connections.Remove(connectionId);
                        if (connections.Count == 0)
                        {
                            _connections.Remove(key);
                        }
                    }
                }
            }
        }
    }
}
