using ChatWpf.Models.Enums;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class FriendRequest
    {
        public required int Id { get; set; }
        public required AppUserInfo From { get; set; }
        public required string RequestMessage { get; set; }
        public required FriendRequestState State { get; set; }
    }
}
