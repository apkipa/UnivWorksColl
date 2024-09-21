using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class MyUserInfo
    {
        public required string UserName { get; set; }
        public required int Id { get; set; }
        public required string Email { get; set; }
    }
}
