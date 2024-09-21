using ChatWpf.Misc;
using ChatWpf.Models.Enums;
using CommunityToolkit.Mvvm.ComponentModel;
using Flurl.Util;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class ChatMsg : ObservableObject
    {
        public required int Id { get; set; }
        public required int SenderId { get; set; }
        public PrincipalNumber? Sender { get; set; }
        public required int ReceiverId { get; set; }
        public PrincipalNumber? Receiver { get; set; }
        public required ChatMsgKind Kind { get; set; }
        public required string Content { get; set; }
        public required DateTimeOffset SendTime { get; set; }
        public AppUserInfo? SenderDetails { get; set; }
        public JsonObject? ReceiverDetails { get; set; }
        public bool IsDeleted { get; set; }

        public bool ReceiverIsSelf { get; set; }
        private bool _isHighlighted;
        public bool IsHighlighted
        {
            get => _isHighlighted;
            set => SetProperty(ref _isHighlighted, value);
        }
        /*private bool _isImageLikeFile;
        public bool IsImageLikeFile
        {
            get => _isImageLikeFile;
            set => SetProperty(ref _isImageLikeFile, value);
        }*/
        public bool IsImageLikeFile => Kind == ChatMsgKind.File && Utils.IsImageFileName(Content);

        public string AsFilePreviewUrl => $"{Utils.HBase}/api/file/preview?fileId={Content.SplitOnFirstOccurence(".")[0]}";

        public string FriendlyContent => Kind switch
        {
            ChatMsgKind.PlainText => Content,
            ChatMsgKind.Image or ChatMsgKind.File => Content[(Content.IndexOf('.') + 1)..],
            _ => Content,
        };
        public AppUserInfo? ReceiverDetailsAsUser => JsonSerializer.Deserialize<AppUserInfo>(ReceiverDetails, Utils.JsonOptions);
        public ChatGroup? ReceiverDetailsAsGroup => JsonSerializer.Deserialize<ChatGroup>(ReceiverDetails, Utils.JsonOptions);

        public string? ReceiverDetailsAsUser_UserName => ReceiverDetailsAsUser?.UserName;
    }
}
