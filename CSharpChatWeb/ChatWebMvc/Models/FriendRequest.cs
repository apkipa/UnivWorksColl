using ChatWebMvc.Models.Enums;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations.Schema;

namespace ChatWebMvc.Models
{
    public class FriendRequest
    {
        public int Id { get; set; }

        public int FromId { get; set; }
        [ForeignKey("FromId")]
        public AppUser From { get; set; } = null!;
        public int ToId { get; set; }
        [ForeignKey("ToId")]
        public AppUser To { get; set; } = null!;

        public DateTimeOffset RequestDate { get; set; }
        public string RequestMessage { get; set; } = string.Empty;
        public FriendRequestState State { get; set; } = FriendRequestState.Pending;
    }
}
