using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class ChatGroup
    {
        public required int Id { get; set; }
        public required string Name { get; set; }
        public required string Description { get; set; }
        public required int OwnerId { get; set; }
        public AppUserInfo? Owner { get; set; }
        public required DateTimeOffset CreationDate { get; set; }
    }
}
