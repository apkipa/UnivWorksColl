using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace ChatWpf.Models.Enums
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
