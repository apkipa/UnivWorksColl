using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class SearchMessagesResult
    {
        public required List<ChatMsg> Messages { get; set; }
        public required int TotalCount { get; set; }
        public required bool HasMore { get; set; }
    }
}
