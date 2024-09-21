using ChatWpf.Misc;
using ChatWpf.Models;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.ViewModel
{
    public class RequestAddFriendDialogVM : ObservableObject
    {
        private AppUserInfo _targetUser = null!;
        public AppUserInfo TargetUser
        {
            get => _targetUser;
            set => SetProperty(ref _targetUser, value);
        }

        public string RequestMessage { get; set; } = string.Empty;

        public AsyncRelayCommand AddFriendCmd => new(async () =>
        {
            await Utils.HClient.Request("api/friend/request-add")
                .PostJsonAndUnwrapAsync(new
                {
                    TargetId = TargetUser.Id,
                    Message = RequestMessage,
                });

            Utils.GrowlSuccess("已发送好友请求");
        });
    }
}
