using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class SearchPrincipalsResult
    {
        public required List<AppUserInfo> Users { get; set; }
        public required List<ChatGroupSlim> Groups { get; set; }
    }
}
