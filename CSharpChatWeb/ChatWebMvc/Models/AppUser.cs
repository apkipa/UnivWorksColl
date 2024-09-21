using Microsoft.AspNetCore.Identity;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace ChatWebMvc.Models
{
    public class AppUser : IdentityUser<int>
    {
        [JsonIgnore]
        public override string? PasswordHash { get; set; }

        //public int PrincipalNumberId { get; set; }
        [ForeignKey("Id")]
        public PrincipalNumber PrincipalNumber { get; set; } = null!;
        public bool IsBanned { get; set; } = false;

        public string Nickname { get; set; } = string.Empty;

        public List<UserFriendship> Friendships { get; set; } = [];
        public List<UserGroupJoin> UserGroupJoins { get; set; } = [];
    }
}
