using ChatWpf.Misc;
using ChatWpf.Models;
using ChatWpf.Models.Enums;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Flurl.Http;
using Microsoft.AspNetCore.SignalR.Client;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Threading;

namespace ChatWpf.ViewModel
{
    public class MainChatPageVM : ObservableObject, IAsyncDisposable
    {
        public ListBox ChatMsgsListBox { get; set; } = null!;

        private readonly Dispatcher _dispatcher = Dispatcher.CurrentDispatcher;

        public class RecentChatEntry : ObservableObject
        {
            private int _id;
            public int Id
            {
                get => _id;
                set => SetProperty(ref _id, value);
            }

            public object Target { get; set; } = null!;

            /*public int Id { get; set; }
            public ChatKind Kind { get; set; }
            public string Name { get; set; } = null!;
            public ChatMsg? LastMessage { get; set; }
            public bool IsSelected { get; set; }*/

            private ChatKind _kind;
            public ChatKind Kind
            {
                get => _kind;
                set => SetProperty(ref _kind, value);
            }

            private string _name = null!;
            public string Name
            {
                get => _name;
                set => SetProperty(ref _name, value);
            }

            private ChatMsg? _lastMessage;
            public ChatMsg? LastMessage
            {
                get => _lastMessage;
                set => SetProperty(ref _lastMessage, value);
            }

            private bool _isSelected;
            public bool IsSelected
            {
                get => _isSelected;
                set => SetProperty(ref _isSelected, value);
            }
        }

        public class SearchPrincipalEntry
        {
            public object Value { get; set; } = null!;

            public ChatKind Kind { get; set; }
            public AppUserInfo? AsUser => Value as AppUserInfo;
            public ChatGroupSlim? AsGroup => Value as ChatGroupSlim;

            public int Id => AsUser?.Id ?? AsGroup?.Id ?? 0;
        }

        public HubConnection HubConnection { get; set; }

        private string _chatSearchText = string.Empty;
        public string ChatSearchText
        {
            get => _chatSearchText;
            set
            {
                SetProperty(ref _chatSearchText, value);

                RecentChatEntriesView.View.Refresh();

                // Reload search data
                ReloadSearchPrincipalsAsync().ObserveUnhandledException();
                ReloadSearchMessagesAsync().ObserveUnhandledException();
            }
        }

        private string _curChatInputText = string.Empty;
        public string CurChatInputText
        {
            get => _curChatInputText;
            set => SetProperty(ref _curChatInputText, value);
        }

        public ObservableCollection<ChatMsg> SearchMessages { get; } = [];
        private bool _searchMessagesNotEmpty;
        public bool SearchMessagesNotEmpty
        {
            get => _searchMessagesNotEmpty;
            set => SetProperty(ref _searchMessagesNotEmpty, value);
        }

        public ObservableCollection<SearchPrincipalEntry> SearchPrincipals { get; } = [];
        private bool _searchPrincipalsNotEmpty;
        public bool SearchPrincipalsNotEmpty
        {
            get => _searchPrincipalsNotEmpty;
            set => SetProperty(ref _searchPrincipalsNotEmpty, value);
        }
        public CollectionViewSource SearchPrincipalsView { get; }

        private int _thisUserId = 0;
        public int ThisUserId
        {
            get => _thisUserId;
            set => SetProperty(ref _thisUserId, value);
        }

        private MyUserInfo _thisUser = null!;
        public MyUserInfo ThisUser
        {
            get => _thisUser;
            set => SetProperty(ref _thisUser, value);
        }

        private List<RecentChatsInfoUserEntry> _recentChatsInfoUsers = null!;
        private List<RecentChatsInfoGroupEntry> _recentChatsInfoGroups = null!;
        /*private RecentChatsInfo _recentChats = null!;
        public RecentChatsInfo RecentChats
        {
            get => _recentChats;
            set => SetProperty(ref _recentChats, value);
        }*/
        private List<RecentChatEntry> _recentChatEntries = null!;
        public List<RecentChatEntry> RecentChatEntries
        {
            get => _recentChatEntries;
            set
            {
                SetProperty(ref _recentChatEntries, value);
                UpdateIsSelected();
            }
        }
        public CollectionViewSource RecentChatEntriesView { get; }

        private RecentChatEntry? _curChatTarget = null;
        public RecentChatEntry? CurChatTarget
        {
            get => _curChatTarget;
            set
            {
                SetProperty(ref _curChatTarget, value);
                UpdateIsSelected();
            }
        }

        private ObservableCollection<RecentChatEntry> _chatSearchTargetList = [];
        public ObservableCollection<RecentChatEntry> ChatSearchTargetList
        {
            get => _chatSearchTargetList;
            set => SetProperty(ref _chatSearchTargetList, value);
        }

        private ObservableCollection<ChatMsg> _curChatTargetMsgs = [];
        public ObservableCollection<ChatMsg> CurChatTargetMsgs
        {
            get => _curChatTargetMsgs;
            set => SetProperty(ref _curChatTargetMsgs, value);
        }

        public AsyncRelayCommand SendMsgCmd => new(async () =>
        {
            if (CurChatTarget == null)
            {
                Utils.GrowlError("请选择一个聊天对象");
                return;
            }

            if (string.IsNullOrWhiteSpace(CurChatInputText))
            {
                return;
            }

            await Utils.HClient.Request("/api/chat/send-msg")
                .PostJsonAndUnwrapAsync(new
                {
                    PrincipalId = CurChatTarget.Id,
                    Content = CurChatInputText,
                    Type = "PlainText",
                });

            CurChatInputText = string.Empty;
        });

        public MainChatPageVM()
        {
            HubConnection = new HubConnectionBuilder()
                .WithUrl("http://localhost:5291/api/hubs/chatHub", options =>
                {
                    // Get cookies from the current session
                    foreach (var cookie in Utils.HCookies)
                    {
                        options.Cookies.Add(new Uri(Utils.HBase), new System.Net.Cookie(cookie.Name, cookie.Value));
                    }
                })
                .Build();

            RecentChatEntriesView = new CollectionViewSource
            {
                Source = RecentChatEntries,
            };
            RecentChatEntriesView.Filter += (sender, e) =>
            {
                if (string.IsNullOrWhiteSpace(ChatSearchText))
                {
                    e.Accepted = true;
                    return;
                }

                var entry = e.Item as RecentChatEntry;
                e.Accepted = entry?.Id.ToString().Contains(ChatSearchText) ?? false;
            };

            SearchPrincipalsView = new CollectionViewSource
            {
                Source = SearchPrincipals,
            };
            SearchPrincipalsView.Filter += (sender, e) =>
            {
                var entry = (e.Item as SearchPrincipalEntry)!;
                e.Accepted = RecentChatEntries.Find(x => x.Id == entry.Id) == null;
            };
        }

        private async Task HandleIncomingMessage(int fromId, int toId, int msgId, string details)
        {
            if (msgId == 0)
            {
                // Just reload recent chats for now
                await ReloadRecentChatAsync();
                return;
            }

            var msgDetails = JsonSerializer.Deserialize<ChatMsg>(details, Utils.JsonOptions);
            if (msgDetails == null)
            {
                Utils.GrowlError("无法解析消息");
                return;
            }

            msgDetails.ReceiverIsSelf = msgDetails.SenderId != ThisUserId;

            // Handle deleted messages
            if (msgDetails.IsDeleted)
            {
                await ReloadRecentChatAsync();

                // Remove from current chat
                if (_curChatTarget?.Id == fromId || _curChatTarget?.Id == toId)
                {
                    var toRemove = CurChatTargetMsgs.FirstOrDefault(x => x.Id == msgDetails.Id);
                    if (toRemove != null)
                    {
                        CurChatTargetMsgs.Remove(toRemove);
                    }
                }

                return;
            }

            // Update recent chats
            int targetId = -1;
            {
                var entry = _recentChatsInfoUsers.FirstOrDefault(x => x.Target.Id == fromId && ThisUserId == toId || x.Target.Id == toId && ThisUserId == fromId);
                if (entry != null)
                {
                    entry.Chat = msgDetails;
                    targetId = entry.Target.Id;
                }
            }
            {
                var entry = _recentChatsInfoGroups.FirstOrDefault(x => x.Target.Id == toId);
                if (entry != null)
                {
                    entry.Chat = msgDetails;
                    targetId = entry.Target.Id;
                }
            }
            if (targetId != -1)
            {
                UpdateRecentChat();
            }
            else
            {
                await ReloadRecentChatAsync();

                {
                    var entry = _recentChatsInfoUsers.FirstOrDefault(x => x.Target.Id == fromId && ThisUserId == toId || x.Target.Id == toId && ThisUserId == fromId);
                    if (entry != null)
                    {
                        targetId = entry.Target.Id;
                    }
                }
                {
                    var entry = _recentChatsInfoGroups.FirstOrDefault(x => x.Target.Id == toId);
                    if (entry != null)
                    {
                        targetId = entry.Target.Id;
                    }
                }
            }

            // Update current chat
            if (_curChatTarget?.Id == targetId)
            {
                // Is current chat scrolled to bottom?
                var scrollViewer = Utils.GetDescendant<ScrollViewer>(ChatMsgsListBox)!;
                var isScrolledToBottom = scrollViewer.VerticalOffset + scrollViewer.ViewportHeight >= scrollViewer.ExtentHeight;

                CurChatTargetMsgs.Add(msgDetails);

                // If previously scrolled to bottom, scroll to the new message
                if (isScrolledToBottom)
                {
                    ChatMsgsListBox.ScrollIntoView(msgDetails);
                }
            }
        }

        public async Task InitAsync()
        {
            HubConnection.On<int, int, int, string>("OnReceiveNewMessage", (fromId, toId, msgId, details) =>
            {
                _dispatcher.Invoke(() => HandleIncomingMessage(fromId, toId, msgId, details).ObserveUnhandledException());
            });
            HubConnection.Closed += async (error) =>
            {
                Utils.GrowlError($"连接已断开: {error}");
            };

            ThisUser = await Utils.HClient.Request("/api/auth/myinfo").GetJsonAndUnwrapAsync<MyUserInfo>();
            ThisUserId = ThisUser.Id;

            await HubConnection.StartAsync();

            await Task.Delay(100);

            await ReloadRecentChatAsync();
        }

        private async Task ReloadRecentChatAsync()
        {
            var recentChats = await Utils.HClient.Request("/api/chat/recent-chats").GetJsonAndUnwrapAsync<RecentChatsInfo>();
            _recentChatsInfoUsers = recentChats.Users;
            _recentChatsInfoGroups = recentChats.Groups;
            foreach (var entry in _recentChatsInfoUsers)
            {
                if (entry.Chat != null)
                {
                    entry.Chat.ReceiverIsSelf = entry.Chat.SenderId != ThisUserId;
                }
            }
            foreach (var entry in _recentChatsInfoGroups)
            {
                if (entry.Chat != null)
                {
                    entry.Chat.ReceiverIsSelf = entry.Chat.SenderId != ThisUserId;
                }
            }
            UpdateRecentChat();
        }

        private void UpdateRecentChat()
        {
            if (_recentChatsInfoUsers == null || _recentChatsInfoGroups == null)
            {
                return;
            }

            var entries = new List<RecentChatEntry>();
            entries.AddRange(_recentChatsInfoUsers.Select(x => new RecentChatEntry
            {
                Id = x.Target.Id,
                Kind = ChatKind.User,
                Name = x.Target.UserName,
                LastMessage = x.Chat,
                Target = x.Target,
            }));
            entries.AddRange(_recentChatsInfoGroups.Select(x => new RecentChatEntry
            {
                Id = x.Target.Id,
                Kind = ChatKind.Group,
                Name = x.Target.Name,
                LastMessage = x.Chat,
                Target = x.Target,
            }));
            entries.Sort((a, b) => b.LastMessage?.SendTime.CompareTo(a.LastMessage?.SendTime ?? default) ?? 0);
            RecentChatEntries = entries;
            RecentChatEntriesView.Source = RecentChatEntries;
            RecentChatEntriesView.View.Refresh();

            SearchPrincipalsView.View.Refresh();
        }

        public void UpdateIsSelected()
        {
            foreach (var entry in _recentChatEntries) {
                entry.IsSelected = entry.Id == _curChatTarget?.Id;
            }
        }

        private async Task ReloadSearchPrincipalsAsync()
        {
            if (string.IsNullOrWhiteSpace(ChatSearchText))
            {
                SearchPrincipals.Clear();
                SearchPrincipalsNotEmpty = SearchPrincipals.Count > 0;
                return;
            }

            var newSearchPrincipals = await Utils.HClient.Request("/api/search/search-principals")
                .SetQueryParams(new
                {
                    Keyword = ChatSearchText,
                })
                .GetJsonAndUnwrapAsync<SearchPrincipalsResult>();

            SearchPrincipals.Clear();
            foreach (var principal in newSearchPrincipals.Users)
            {
                if (principal.Id == ThisUserId)
                {
                    continue;
                }
                SearchPrincipals.Add(new SearchPrincipalEntry
                {
                    Kind = ChatKind.User,
                    Value = principal,
                });
            }
            foreach (var principal in newSearchPrincipals.Groups)
            {
                SearchPrincipals.Add(new SearchPrincipalEntry
                {
                    Kind = ChatKind.Group,
                    Value = principal,
                });
            }

            SearchPrincipalsNotEmpty = SearchPrincipals.Count > 0;
        }

        private async Task ReloadSearchMessagesAsync()
        {
            if (string.IsNullOrWhiteSpace(ChatSearchText))
            {
                SearchMessages.Clear();
                SearchMessagesNotEmpty = SearchMessages.Count > 0;
                return;
            }

            var newSearchMsgs = await Utils.HClient.Request("/api/search/search-msgs")
                .SetQueryParams(new
                {
                    PrincipalId = ChatSearchTargetList.FirstOrDefault()?.Id,
                    Keyword = ChatSearchText,
                    MsgId = (int?)null,
                    MsgCount = 999999,
                })
                .GetJsonAndUnwrapAsync<SearchMessagesResult>();

            SearchMessages.Clear();
            foreach (var msg in newSearchMsgs.Messages)
            {
                //msg.ReceiverIsSelf = msg.ReceiverId == ThisUserId;
                msg.ReceiverIsSelf = msg.SenderId != ThisUserId;
                SearchMessages.Add(msg);
            }

            SearchMessagesNotEmpty = SearchMessages.Count > 0;
        }

        public async Task UpdateCurChatTargetAsync(RecentChatEntry? entry, ChatMsg? chatMsg, bool highlightMsg = false)
        {
            if (entry == null)
            {
                CurChatTarget = null;
                CurChatTargetMsgs.Clear();
                return;
            }

            if (CurChatTarget?.Id != entry.Id)
            {
                // Changed chat target, need to reload messages
                CurChatTargetMsgs.Clear();
            }

            CurChatTarget = entry;

            if (chatMsg == null)
            {
                // No initial message, load all
                var resp = await Utils.HClient.Request("/api/chat/get-msgs")
                    .SetQueryParams(new
                    {
                        PrincipalId = entry.Id,
                        MsgId = (int?)null,
                        Direction = "before",
                        MsgCount = 999999,
                    })
                    .GetJsonAndUnwrapAsync<ChatGetMsgsResult>();
                foreach (var msg in resp.Messages)
                {
                    msg.ReceiverIsSelf = msg.SenderId != ThisUserId;
                    CurChatTargetMsgs.Add(msg);
                }

                // Scroll to the last message
                if (CurChatTargetMsgs.Count > 0)
                {
                    ChatMsgsListBox.ScrollIntoView(CurChatTargetMsgs.Last());
                }
            }
            else
            {
                if (chatMsg.IsDeleted)
                {
                    // Deleted message, remove from list
                    CurChatTargetMsgs.Remove(chatMsg);
                    await ReloadRecentChatAsync();
                    return;
                }

                // No need to reload messages if already exists
                if (CurChatTargetMsgs.FirstOrDefault(x => x.Id == chatMsg.Id) is ChatMsg foundMsg)
                {
                    chatMsg = foundMsg;
                }
                else
                {
                    // Has initial message, load before & after
                    CurChatTargetMsgs.Clear();
                    CurChatTargetMsgs.Add(chatMsg);
                    var resp = await Utils.HClient.Request("/api/chat/get-msgs")
                        .SetQueryParams(new
                        {
                            PrincipalId = entry.Id,
                            MsgId = chatMsg.Id,
                            Direction = "after",
                            MsgCount = 999999,
                        })
                        .GetJsonAndUnwrapAsync<ChatGetMsgsResult>();
                    CurChatTargetMsgs.Clear();
                    foreach (var msg in resp.Messages)
                    {
                        msg.ReceiverIsSelf = msg.SenderId != ThisUserId;
                        CurChatTargetMsgs.Add(msg);
                    }
                    if (CurChatTargetMsgs.Count > 0)
                    {
                        chatMsg = CurChatTargetMsgs.First();
                        ChatMsgsListBox.ScrollIntoView(chatMsg);
                    }

                    resp = await Utils.HClient.Request("/api/chat/get-msgs")
                        .SetQueryParams(new
                        {
                            PrincipalId = entry.Id,
                            MsgId = chatMsg.Id,
                            Direction = "before",
                            MsgCount = 999999,
                        })
                        .GetJsonAndUnwrapAsync<ChatGetMsgsResult>();
                    if (resp.Messages.Count > 0 && CurChatTargetMsgs.Count > 0)
                    {
                        if (resp.Messages.Last().Id == CurChatTargetMsgs.First().Id)
                        {
                            // Skip the duplicate message
                            resp.Messages.RemoveAt(resp.Messages.Count - 1);
                        }
                    }
                    resp.Messages.Reverse();
                    foreach (var msg in resp.Messages)
                    {
                        msg.ReceiverIsSelf = msg.SenderId != ThisUserId;
                        CurChatTargetMsgs.Insert(0, msg);
                    }
                    /*if (CurChatTargetMsgs.Count > 0)
                    {
                        ChatMsgsListBox.ScrollIntoView(chatMsg);
                    }*/
                }

                ChatMsgsListBox.ScrollIntoView(chatMsg);
                if (highlightMsg)
                {
                    // Highlight the message
                    chatMsg.IsHighlighted = false;
                    chatMsg.IsHighlighted = true;
                }
            }
        }

        public async ValueTask DisposeAsync()
        {
            await HubConnection.DisposeAsync();

            GC.SuppressFinalize(this);
        }
    }
}
