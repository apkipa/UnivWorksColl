using ChatWpf.Models;
using ChatWpf.Models.Enums;
using CommunityToolkit.Mvvm.Input;
using HandyControl.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ChatWpf.Controls
{
    /// <summary>
    /// Interaction logic for ChatBubble.xaml
    /// </summary>
    public class ChatBubble : SelectableItem
    {
        public ChatBubbleUserKind UserKind
        {
            get { return (ChatBubbleUserKind)GetValue(UserKindProperty); }
            set { SetValue(UserKindProperty, value); }
        }
        public static readonly DependencyProperty UserKindProperty = DependencyProperty.Register(
            nameof(UserKind), typeof(ChatBubbleUserKind), typeof(ChatBubble), new PropertyMetadata(ChatBubbleUserKind.Others));

        public ChatBubbleKind BubbleKind
        {
            get { return (ChatBubbleKind)GetValue(BubbleKindProperty); }
            set { SetValue(BubbleKindProperty, value); }
        }
        public static readonly DependencyProperty BubbleKindProperty = DependencyProperty.Register(
            nameof(BubbleKind), typeof(ChatBubbleKind), typeof(ChatBubble), new PropertyMetadata(ChatBubbleKind.PlainText));

        public bool UsePreviewImage
        {
            get { return (bool)GetValue(UsePreviewImageProperty); }
            set { SetValue(UsePreviewImageProperty, value); }
        }
        public static readonly DependencyProperty UsePreviewImageProperty = DependencyProperty.Register(
            nameof(UsePreviewImage), typeof(bool), typeof(ChatBubble), new PropertyMetadata(false));

        public bool ShowSender
        {
            get { return (bool)GetValue(ShowSenderProperty); }
            set { SetValue(ShowSenderProperty, value); }
        }
        public static readonly DependencyProperty ShowSenderProperty = DependencyProperty.Register(
            nameof(ShowSender), typeof(bool), typeof(ChatBubble), new PropertyMetadata(false));

        public ChatMsg ChatMsg
        {
            get { return (ChatMsg)GetValue(ChatMsgProperty); }
            set {
                SetValue(ChatMsgProperty, value);
                /*BubbleKind = ChatMsg.Kind switch
                {
                    ChatMsgKind.PlainText => ChatBubbleKind.PlainText,
                    ChatMsgKind.File => ChatBubbleKind.File,
                    ChatMsgKind.Image => ChatBubbleKind.Image,
                    _ => ChatBubbleKind.PlainText,
                };*/
            }
        }

        public static readonly DependencyProperty ChatMsgProperty = DependencyProperty.Register(
            nameof(ChatMsg), typeof(ChatMsg), typeof(ChatBubble), new PropertyMetadata(null));

        public Action<ChatMsg>? DownloadFileAct { get; set; }
        public RelayCommand<ChatMsg> DownloadFileCmd => new((x) => DownloadFileAct?.Invoke(x!));

        public Action<ChatMsg>? PreviewImageAct { get; set; }
        public RelayCommand<MouseButtonEventArgs> PreviewImageCmd => new((e) =>
        {
            // Don't handle right mouse click
            if (e!.ChangedButton != MouseButton.Left)
            {
                return;
            }

            PreviewImageAct?.Invoke(ChatMsg);
        });
    }
}
