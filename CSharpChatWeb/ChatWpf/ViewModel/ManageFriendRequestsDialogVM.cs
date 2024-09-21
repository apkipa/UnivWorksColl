using ChatWpf.Misc;
using ChatWpf.Models;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Flurl.Http;
using HandyControl.Tools.Extension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.ViewModel
{
    public class ManageFriendRequestsDialogVM : ObservableObject
    {
        public MainChatPageVM MainChatPageVM { get; set; } = null!;

        private List<FriendRequest> _friendRequests = null!;
        public List<FriendRequest> FriendRequests
        {
            get => _friendRequests;
            set => SetProperty(ref _friendRequests, value);
        }

        public async Task InitAsync()
        {
            ArgumentNullException.ThrowIfNull(MainChatPageVM, nameof(MainChatPageVM));

            FriendRequests = await Utils.HClient.Request("/api/friend/my-friend-requests").GetJsonAndUnwrapAsync<List<FriendRequest>>();
        }

        public RelayCommand<FriendRequest> AcceptFriendRequestCommand => new(async (request) =>
        {
            ArgumentNullException.ThrowIfNull(request, nameof(request));

            await Utils.HClient.Request($"/api/friend/accept-add").PostJsonAndUnwrapAsync(new
            {
                FriendRequestId = request.Id,
            });
            Utils.GrowlSuccess("已接受好友请求");

            await InitAsync();
        });

        public RelayCommand<FriendRequest> RejectFriendRequestCommand => new(async (request) =>
        {
            ArgumentNullException.ThrowIfNull(request, nameof(request));

            await Utils.HClient.Request($"/api/friend/reject-add").PostJsonAndUnwrapAsync(new
            {
                FriendRequestId = request.Id,
            });
            Utils.GrowlSuccess("已拒绝好友请求");

            await InitAsync();
        });
    }
}
