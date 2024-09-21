using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace ChatWpf.Models.Enums
{
    [JsonConverter(typeof(JsonStringEnumConverter))]
    public enum PrincipalKind
    {
        Unknown,
        User = 1,
        Group,
    }
}
