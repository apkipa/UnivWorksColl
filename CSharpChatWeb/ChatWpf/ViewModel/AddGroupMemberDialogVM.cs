using ChatWpf.Misc;
using ChatWpf.Models;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace ChatWpf.ViewModel
{
    public class AddGroupMemberDialogVM : ObservableObject
    {
        public MainChatPageVM MainChatPageVM { get; set; } = null!;
        public ManageGroupDialogVM ManageGroupDialogVM { get; set; } = null!;

        private List<AppUserInfo> _friendsList = [];
        public List<AppUserInfo> FriendsList
        {
            get => _friendsList;
            set => SetProperty(ref _friendsList, value);
        }
        public CollectionViewSource FriendsListCVS { get; } = new();

        public AddGroupMemberDialogVM()
        {
            FriendsListCVS.Filter += (sender, e) =>
            {
                if (e.Item is AppUserInfo user)
                {
                    e.Accepted = !ManageGroupDialogVM.GroupMembers.Any(x => x.Id == user.Id);
                }
            };
        }

        public async Task InitAsync()
        {
            ArgumentNullException.ThrowIfNull(MainChatPageVM, nameof(MainChatPageVM));
            ArgumentNullException.ThrowIfNull(ManageGroupDialogVM, nameof(ManageGroupDialogVM));

            FriendsList = await Utils.HClient.Request("/api/friend/my-friends").GetJsonAndUnwrapAsync<List<AppUserInfo>>();
            FriendsListCVS.Source = FriendsList;
        }

        public AsyncRelayCommand<AppUserInfo> AddMemberCmd => new(async (user) =>
        {
            await Utils.HClient.Request("/api/group/add-member").PostJsonAndUnwrapAsync(new
            {
                ManageGroupDialogVM.GroupId,
                UserId = user!.Id,
            });
            Utils.GrowlSuccess("已添加成员");

            await ManageGroupDialogVM.ReloadGroupMembersAsync();
        });
    }
}
