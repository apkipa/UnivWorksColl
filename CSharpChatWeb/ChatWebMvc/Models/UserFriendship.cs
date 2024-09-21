using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations.Schema;

namespace ChatWebMvc.Models
{
    [PrimaryKey("FromId", "ToId")]
    public class UserFriendship
    {
        public int FromId { get; set; }
        [ForeignKey("FromId")]
        public AppUser From { get; set; } = null!;
        public int ToId { get; set; }
        [ForeignKey("ToId")]
        public AppUser To { get; set; } = null!;

        public DateTimeOffset FriendshipDate { get; set; }

        public int? LastWatchedMsgId { get; set; }
        [ForeignKey("LastWatchedMsgId")]
        public ChatMsg? LastWatchedMsg { get; set; }
    }
}
