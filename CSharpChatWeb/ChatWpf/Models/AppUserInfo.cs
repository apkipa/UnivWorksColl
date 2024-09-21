using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class AppUserInfo
    {
        public required int Id { get; set; }
        public required string UserName { get; set; }
        public required string Nickname { get; set; }
    }
}
