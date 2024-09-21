using System.Text.Json.Serialization;

namespace ChatWebMvc.Models.Enums
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public enum ChatMsgKind
    {
        Unknown = 0,
        PlainText = 1,
        Image = 2,
        File = 3,
        RichXml = 4,
    }
}
