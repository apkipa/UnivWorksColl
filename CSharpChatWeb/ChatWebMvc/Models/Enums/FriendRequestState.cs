using System.Text.Json.Serialization;

namespace ChatWebMvc.Models.Enums
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public enum FriendRequestState
    {
        Pending,
        Accepted,
        Rejected,
    }
}
