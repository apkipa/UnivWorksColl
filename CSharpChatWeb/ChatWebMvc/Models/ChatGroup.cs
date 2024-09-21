using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace ChatWebMvc.Models
{
    public class ChatGroup
    {
        public int Id { get; set; }
        [ForeignKey("Id")]
        public PrincipalNumber PrincipalNumber { get; set; } = null!;

        public string Name { get; set; } = string.Empty;
        public string Description { get; set; } = string.Empty;

        public int OwnerId { get; set; }
        [ForeignKey("OwnerId")]
        public AppUser? Owner { get; set; }

        public DateTimeOffset CreationDate { get; set; }
        [JsonIgnore]
        public List<UserGroupJoin> UserGroupJoins { get; set; } = [];
    }
}
