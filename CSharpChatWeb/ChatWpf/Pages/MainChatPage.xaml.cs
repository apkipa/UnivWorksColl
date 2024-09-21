using ChatWpf.Misc;
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
using Microsoft.AspNetCore.SignalR.Client;
using HandyControl.Controls;
using HandyControl.Tools.Extension;
using ChatWpf.Controls;
using ChatWpf.ViewModel;
using Flurl.Http;
using Flurl;
using ChatWpf.Models;
using ChatWpf.Models.Enums;
using Flurl.Util;
using System.Security.Policy;
using System.Reflection;
using System.Windows.Controls.Primitives;

namespace ChatWpf.Pages
{
    /// <summary>
    /// Interaction logic for MainChatPage.xaml
    /// </summary>
    public partial class MainChatPage : Page
    {
        public MainChatPage()
        {
            InitializeComponent();

            VM.ChatMsgsListBox = ChatMsgsListBox;

            InitAsync();

            // Manually set DataContext for ContextMenu; WPF doesn't do this until right click
            // https://stackoverflow.com/q/5231452
            ChatCtxMenu.DataContext = DataContext;
        }

        private async void InitAsync()
        {
            await VM.InitAsync();

            ChatLeftLoadingContainer.Child = null;
            ChatLeftLoadingContainer.Background = null;

            /*async void ScrollViewer_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
            {
                var scrollViewer = (System.Windows.Controls.ScrollViewer)sender;
                scrollViewer.IsVisibleChanged -= ScrollViewer_IsVisibleChanged;

                await Task.Delay(1000);

                var prop = scrollViewer.GetType().GetProperty("ScrollInfo", BindingFlags.NonPublic | BindingFlags.Instance)!;
                prop.SetValue(scrollViewer, new ScrollInfoAdapter((IScrollInfo)prop.GetValue(scrollViewer)!));
            }

            var scrollViewer = Utils.GetDescendant<System.Windows.Controls.ScrollViewer>(ChatMsgsListBox)!;
            scrollViewer.IsVisibleChanged += ScrollViewer_IsVisibleChanged;*/
        }

        private void DrawerCloseButton_Click(object sender, RoutedEventArgs e)
        {
            DrawerLeft.IsOpen = false;
        }

        private void DrawerOpenButton_Click(object sender, RoutedEventArgs e)
        {
            DrawerLeft.IsOpen = true;
        }

        private void LogoutButton_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GoBack();
        }

        private async void CreateNewGroupButton_Click(object sender, RoutedEventArgs e)
        {
            DrawerLeft.IsOpen = false;

            var groupName = await Dialog.Show<CreateNewGroupDialog>()
                .Initialize<CreateNewGroupDialogVM>(vm => vm.Result = "")
                .GetResultAsync<string>();
            if (groupName == null)
            {
                return;
            }

            var resp = await Utils.HClient.Request("/api/group/create")
                .PostJsonAndUnwrapAsync<ChatGroup>(new { GroupName = groupName });
        }

        private void ManageFriendRequestsButton_Click(object sender, RoutedEventArgs e)
        {
            DrawerLeft.IsOpen = false;

            var dialog = Dialog.Show<ManageFriendRequestsDialog>()
                .Initialize<ManageFriendRequestsDialogVM>(vm =>
                {
                    vm.MainChatPageVM = VM;
                    vm.InitAsync().ObserveUnhandledException();
                });
            dialog.VerticalContentAlignment = VerticalAlignment.Stretch;
        }

        private async void RecentChatListItem_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            // Don't handle right mouse click
            if (e.ChangedButton != MouseButton.Left)
            {
                return;
            }

            var item = (sender as FrameworkElement)?.DataContext as MainChatPageVM.RecentChatEntry;
            if (item == VM.CurChatTarget)
            {
                item = null;
            }

            //VM.CurChatTarget = item;
            await VM.UpdateCurChatTargetAsync(item, item?.LastMessage);
            VM.UpdateIsSelected();
        }

        private void SearchPrincipalItem_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            // Don't handle right mouse click
            if (e.ChangedButton != MouseButton.Left)
            {
                return;
            }

            var entry = ((sender as FrameworkElement)?.DataContext as MainChatPageVM.SearchPrincipalEntry)!;
            if (entry.Kind == ChatKind.Group)
            {
                Utils.GrowlInfo("你不能直接加入此群组。请让此群组的管理员邀请你加入群。");
                return;
            }

            var dialog = Dialog.Show<RequestAddFriendDialog>()
                .Initialize<RequestAddFriendDialogVM>(vm =>
                {
                    vm.TargetUser = entry.AsUser!;
                });
        }

        private async void SearchChatMsgItem_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            // Don't handle right mouse click
            if (e.ChangedButton != MouseButton.Left)
            {
                return;
            }

            var entry = ((sender as FrameworkElement)?.DataContext as ChatMsg)!;
            var chatTarget = VM.RecentChatEntries.FirstOrDefault(x => x.Id == entry.SenderId || x.Id == entry.ReceiverId);
            if (chatTarget == null)
            {
                Utils.GrowlError("未找到对应的聊天对象");
                return;
            }
            await VM.UpdateCurChatTargetAsync(chatTarget, entry, true);
        }

        private void ChatMsgMenuCopy_Click(object sender, RoutedEventArgs e)
        {
            var entry = ((sender as FrameworkElement)?.DataContext as ChatMsg)!;
            Clipboard.SetText(entry.FriendlyContent);
        }

        private async void ChatMsgMenuDelete_Click(object sender, RoutedEventArgs e)
        {
            var entry = ((sender as FrameworkElement)?.DataContext as ChatMsg)!;

            var confirmResult = await ConfirmDialog.ShowAsync("删除消息?",
                "确实要删除此消息吗? 此操作不可逆。",
                "WarningGeometry",
                "删除", "取消");
            if (confirmResult == ConfirmDialogResult.Primary)
            {
                await Utils.HClient.Request("/api/chat/delete-msg")
                    .PostJsonAndUnwrapAsync(new { MsgId = entry.Id });

                Utils.GrowlSuccess("消息已删除");
            }
        }

        private async void ChatSendMsgButton_Click(object sender, RoutedEventArgs e)
        {
            await VM.SendMsgCmd.ExecuteAsync(null);
        }

        private async void ChatMsgBubble_DoDownload(ChatMsg chatMsg)
        {
            var fileId = chatMsg.Content.SplitOnFirstOccurence(".")[0];
            var fileName = chatMsg.FriendlyContent;

            // Open save file dialog
            var saveFileDialog = new Microsoft.Win32.SaveFileDialog
            {
                FileName = fileName,
                DefaultExt = System.IO.Path.GetExtension(fileName),
                Filter = "All files (*.*)|*.*",
            };
            if (saveFileDialog.ShowDialog() != true)
            {
                return;
            }

            // Download file
            var savePath = saveFileDialog.FileName;
            await Utils.HClient.Request("/api/file/download")
                .WithCookies(Utils.HCookies)
                .SetQueryParam("fileId", fileId)
                .DownloadFileAsync(savePath[..savePath.LastIndexOf('\\')], savePath[(savePath.LastIndexOf('\\') + 1)..]);

            Utils.GrowlSuccess($"文件已保存到 {savePath}");
        }

        private void ChatMsgBubble_DoPreviewImage(ChatMsg chatMsg)
        {
            var fileId = chatMsg.Content.SplitOnFirstOccurence(".")[0];
            var fileName = chatMsg.FriendlyContent;

            //new ImageBrowser(new Uri($"{Utils.HBase}/api/file/preview?fileId={fileId}")).Show();

            async void Browser_Loaded(object sender, RoutedEventArgs e)
            {
                var browser = (ImageBrowser)sender;
                browser.Loaded -= Browser_Loaded;

                // Use reflection to get inner _imageViewer
                Type imageBrowserType = typeof(ImageBrowser);
                MethodInfo getTemplateChildMethod = imageBrowserType.GetMethod("GetTemplateChild", BindingFlags.NonPublic | BindingFlags.Instance)!;
                var childElementName = "PART_ImageViewer";
                var imageViewer = (getTemplateChildMethod.Invoke(browser, new object[] { childElementName }) as ImageViewer)!;

                var resp = await Utils.HClient.Request(chatMsg.AsFilePreviewUrl).WithCookies(Utils.HCookies).GetAsync();
                var stm = await resp.ResponseMessage.Content.ReadAsStreamAsync();

                var frame = BitmapFrame.Create(stm, BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.OnLoad);
                imageViewer.ImageSource = frame;

                Type imageViewerType = typeof(ImageViewer);
                PropertyInfo propImgPath = imageViewerType.GetProperty("ImgPath", BindingFlags.NonPublic | BindingFlags.Instance)!;
                propImgPath.SetValue(imageViewer, fileName);

                PropertyInfo propImgSize = imageViewerType.GetProperty("ImgSize", BindingFlags.NonPublic | BindingFlags.Instance)!;
                propImgSize.SetValue(imageViewer, stm.Position);
            }

            var browser = new ImageBrowser
            {
                Topmost = false,
                ResizeMode = ResizeMode.CanResizeWithGrip,
            };
            browser.Loaded += Browser_Loaded;
            browser.Show();
        }

        private async void ChatSendFileButton_Click(object sender, RoutedEventArgs e)
        {
            // Open load file dialog
            var openFileDialog = new Microsoft.Win32.OpenFileDialog
            {
                DefaultExt = "*",
                Filter = "All files (*.*)|*.*",
            };
            if (openFileDialog.ShowDialog() != true)
            {
                return;
            }

            var curChatTargetId = VM.CurChatTarget!.Id;

            // Upload file
            var filePath = openFileDialog.FileName;
            var fileName = System.IO.Path.GetFileName(filePath);
            var resp = await Utils.HClient.Request("/api/file/upload")
                .WithCookies(Utils.HCookies)
                .PostMultipartAndUnwrapAsync<FileUploadResult>(mp => mp.AddFile("file", filePath));

            // Send file message
            await Utils.HClient.Request("/api/chat/send-msg")
                .PostJsonAndUnwrapAsync(new
                {
                    PrincipalId = curChatTargetId,
                    Content = $"{resp.FileId}.{fileName}",
                    Type = "File",
                });
        }

        private void ChatSetAsSearchTargetButton_Click(object sender, RoutedEventArgs e)
        {
            if (VM.ChatSearchTargetList.Count == 0)
            {
                // Major change; reset search text
                VM.ChatSearchText = string.Empty;
            }

            VM.ChatSearchTargetList.Clear();
            VM.ChatSearchTargetList.Add(VM.CurChatTarget!);
        }

        private void ResetChatSearchTargetButton_Click(object sender, RoutedEventArgs e)
        {
            if (VM.ChatSearchTargetList.Count > 0)
            {
                // Major change; reset search text
                VM.ChatSearchText = string.Empty;
            }

            VM.ChatSearchTargetList.Clear();
        }

        private async void ChatCtxMenuDeleteFriend_Click(object sender, RoutedEventArgs e)
        {
            var confirmResult = await ConfirmDialog.ShowAsync("删除好友?",
                "确实要删除此好友吗? 此操作不可逆。",
                "WarningGeometry",
                "删除", "取消");
            if (confirmResult == ConfirmDialogResult.Primary)
            {
                await Utils.HClient.Request("/api/friend/remove-friend")
                    .PostJsonAndUnwrapAsync(new { FriendId = VM.CurChatTarget!.Id });

                await VM.UpdateCurChatTargetAsync(null, null);

                Utils.GrowlSuccess("好友已删除");
            }
        }

        private void ChatCtxMenuManageGroup_Click(object sender, RoutedEventArgs e)
        {
            var dialog = Dialog.Show<ManageGroupDialog>()
                .Initialize<ManageGroupDialogVM>(vm =>
                {
                    vm.MainChatPageVM = VM;
                    vm.InitAsync().ObserveUnhandledException();
                });
            dialog.VerticalContentAlignment = VerticalAlignment.Stretch;
        }

        private async void ChatCtxMenuLeaveGroup_Click(object sender, RoutedEventArgs e)
        {
            var confirmResult = await ConfirmDialog.ShowAsync("离开群组?",
                "确实要离开此群组吗?",
                "WarningGeometry",
                "离开", "取消");
            if (confirmResult == ConfirmDialogResult.Primary)
            {
                await Utils.HClient.Request("/api/group/remove-member")
                    .PostJsonAndUnwrapAsync(new { GroupId = VM.CurChatTarget!.Id, UserId = VM.ThisUserId });

                await VM.UpdateCurChatTargetAsync(null, null);

                Utils.GrowlSuccess("已离开群组");
            }
        }
    }
}
