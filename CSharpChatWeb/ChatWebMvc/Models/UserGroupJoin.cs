using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations.Schema;

namespace ChatWebMvc.Models
{
    [PrimaryKey("UserId", "GroupId")]
    public class UserGroupJoin
    {
        public int UserId { get; set; }
        [ForeignKey("UserId")]
        public AppUser User { get; set; } = null!;
        public int GroupId { get; set; }
        [ForeignKey("GroupId")]
        public ChatGroup Group { get; set; } = null!;

        public DateTimeOffset JoinDate { get; set; }
        public bool IsOperator { get; set; } = false;

        public int? LastWatchedMsgId { get; set; }
        [ForeignKey("LastWatchedMsgId")]
        public ChatMsg? LastWatchedMsg { get; set; }
    }
}
