using System.Text.Json.Serialization;

namespace ChatWebMvc.Models.Enums
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public enum PrincipalKind
    {
        Unknown,
        User = 1,
        Group,
    }
}
