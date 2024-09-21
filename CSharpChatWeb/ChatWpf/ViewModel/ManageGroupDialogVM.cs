using ChatWpf.Controls;
using ChatWpf.Misc;
using ChatWpf.Models;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Flurl.Http;
using HandyControl.Controls;
using HandyControl.Tools.Extension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace ChatWpf.ViewModel
{
    public class ManageGroupDialogVM : ObservableObject
    {
        public MainChatPageVM MainChatPageVM { get; set; } = null!;

        private List<GroupListMemberEntry> _groupMembers = null!;
        public List<GroupListMemberEntry> GroupMembers
        {
            get => _groupMembers;
            set => SetProperty(ref _groupMembers, value);
        }

        private int _groupId;
        public int GroupId
        {
            get => _groupId;
            set => SetProperty(ref _groupId, value);
        }

        private string _groupName = string.Empty;
        public string GroupName
        {
            get => _groupName;
            set => SetProperty(ref _groupName, value);
        }

        private int _ownerId;
        public int OwnerId
        {
            get => _ownerId;
            set => SetProperty(ref _ownerId, value);
        }

        private ChatGroup? _curGroup;
        public ChatGroup? CurGroup
        {
            get => _curGroup;
            set => SetProperty(ref _curGroup, value);
        }

        private bool _meIsOperator;
        public bool MeIsOperator
        {
            get => _meIsOperator;
            set => SetProperty(ref _meIsOperator, value);
        }

        public async Task InitAsync()
        {
            ArgumentNullException.ThrowIfNull(MainChatPageVM, nameof(MainChatPageVM));

            GroupId = MainChatPageVM.CurChatTarget!.Id;
            GroupName = MainChatPageVM.CurChatTarget.Name;
            CurGroup = MainChatPageVM.CurChatTarget.Target as ChatGroup;
            OwnerId = CurGroup!.OwnerId;

            await ReloadGroupMembersAsync();

            MeIsOperator = GroupMembers.Any(m => m.Id == MainChatPageVM.ThisUserId && m.IsOperator);
        }

        public async Task ReloadGroupMembersAsync()
        {
            var members = await Utils.HClient.Request("/api/group/list-members")
                .SetQueryParams(new
                {
                    GroupId,
                })
                .GetJsonAndUnwrapAsync<List<GroupListMemberEntry>>();
            foreach (var member in members)
            {
                member.IsOwner = member.Id == OwnerId;
            }
            GroupMembers = members;
        }

        public RelayCommand AddMemberCmd => new(() =>
        {
            var dialog = Dialog.Show<AddGroupMemberDialog>().Initialize<AddGroupMemberDialogVM>(vm =>
            {
                vm.MainChatPageVM = MainChatPageVM;
                vm.ManageGroupDialogVM = this;
                vm.InitAsync().ObserveUnhandledException();
            });
            dialog.VerticalAlignment = VerticalAlignment.Stretch;
        });

        public AsyncRelayCommand<GroupListMemberEntry> SwitchOperatorCmd => new(async (e) =>
        {
            if (!MeIsOperator)
            {
                return;
            }

            if (e!.IsOperator)
            {
                await Utils.HClient.Request("/api/group/unset-operator")
                    .PostJsonAndUnwrapAsync(new
                    {
                        GroupId,
                        UserId = e.Id,
                    });
            }
            else
            {
                await Utils.HClient.Request("/api/group/set-operator")
                    .PostJsonAndUnwrapAsync(new
                    {
                        GroupId,
                        UserId = e.Id,
                    });
            }

            await ReloadGroupMembersAsync();
        });

        public AsyncRelayCommand<GroupListMemberEntry> RemoveMemberCmd => new(async (e) =>
        {
            if (!MeIsOperator)
            {
                return;
            }

            var confirmResult = await ConfirmDialog.ShowAsync("移除成员?", $"确实要移除 `{e!.UserName}` 吗?", "WarningGeometry", "移除", "取消");
            if (confirmResult != Models.Enums.ConfirmDialogResult.Primary)
            {
                return;
            }

            await Utils.HClient.Request("/api/group/remove-member")
                .PostJsonAndUnwrapAsync(new
                {
                    GroupId,
                    UserId = e.Id,
                });
            Utils.GrowlSuccess("已移除成员");

            await ReloadGroupMembersAsync();
        });

        public AsyncRelayCommand DeleteGroupCmd => new(async () =>
        {
            if (!MeIsOperator)
            {
                Utils.GrowlError("只有管理员才能删除群");
                return;
            }

            var confirmResult = await ConfirmDialog.ShowAsync("删除群组?", "确实要删除此群组吗? 此操作不可逆。", "WarningGeometry", "删除", "取消");
            if (confirmResult != Models.Enums.ConfirmDialogResult.Primary)
            {
                throw new OperationCanceledException();
            }

            await Utils.HClient.Request("/api/group/delete")
                .PostJsonAndUnwrapAsync(new
                {
                    GroupId,
                });
            Utils.GrowlSuccess("已删除群");

            await MainChatPageVM.UpdateCurChatTargetAsync(null, null);
        });
    }
}
