using ChatWebMvc.Misc;
using ChatWebMvc.Models.Enums;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace ChatWebMvc.Models
{
    public class ChatMsg : ISoftDelete
    {
        public int Id { get; set; }

        public int SenderId { get; set; }
        // If null, represents a system message
        public PrincipalNumber? Sender { get; set; } = null!;
        public int ReceiverId { get; set; }
        public PrincipalNumber Receiver { get; set; } = null!;

        public ChatMsgKind Kind { get; set; } = ChatMsgKind.PlainText;
        // NOTE: For Image & File, Content is $"{SHA-256 of the file}.{file name, including extension}",
        //       which is returned by the server when uploading
        public string Content { get; set; } = string.Empty;
        public DateTimeOffset SendTime { get; set; }

        public bool IsDeleted { get; set; }
        public DateTimeOffset? DeletedAt { get; set; }
    }
}
