using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class RecentChatsInfoUserEntry
    {
        public required AppUserInfo Target { get; set; }
        public ChatMsg? Chat { get; set; }
    }

    public class RecentChatsInfoGroupEntry
    {
        public required ChatGroup Target { get; set; }
        public ChatMsg? Chat { get; set; }
    }

    public class RecentChatsInfo
    {
        public required List<RecentChatsInfoUserEntry> Users { get; set; }
        public required List<RecentChatsInfoGroupEntry> Groups { get; set; }
    }
}
