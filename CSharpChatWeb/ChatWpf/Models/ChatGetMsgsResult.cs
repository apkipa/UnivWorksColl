using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class ChatGetMsgsResult
    {
        public required bool HasMore { get; set; }
        public required List<ChatMsg> Messages { get; set; }
    }
}
