using ChatWebMvc.Data;
using ChatWebMvc.Misc;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Hubs
{
    [Authorize]
    public class ChatHub : Hub
    {
        private readonly ApplicationDbContext _context;
        private readonly IConnectionMapping<int> _connectionMapping;
        private readonly ILogger<ChatHub> _logger;

        public ChatHub(ApplicationDbContext context, IConnectionMapping<int> connectionMapping, ILogger<ChatHub> logger)
        {
            _context = context;
            _connectionMapping = connectionMapping;
            _logger = logger;
        }

        public async Task AddToGroupAsync(string groupName)
        {
            await Groups.AddToGroupAsync(Context.ConnectionId, groupName);
        }

        public async Task RemoveFromGroupAsync(string groupName)
        {
            await Groups.RemoveFromGroupAsync(Context.ConnectionId, groupName);
        }

        public async Task NotifyNewMessage(int fromId, int toId, int msgId, string details)
        {
            await Clients.All.SendAsync("OnReceiveNewMessage", fromId, toId, msgId, details);
        }

        public override async Task OnConnectedAsync()
        {
            _logger.LogInformation("User connected: {0}", Context.User?.GetUserId());

            await base.OnConnectedAsync();

            var curUserId = Context.User?.GetUserId() ?? throw new Exception("User not found");
            _connectionMapping.Add(curUserId, Context.ConnectionId);

            // Add current user to their joined groups & friends
            var targetNames = await _context.UserGroupJoins
                .Where(x => x.UserId == curUserId)
                .Select(x => x.Group.Id.ToString())
                .Concat(_context.UserFriendships
                    .Where(x => x.FromId == curUserId)
                    .Select(x => x.ToId.ToString()))
                .ToListAsync();

            foreach (var targetName in targetNames)
            {
                await Groups.AddToGroupAsync(Context.ConnectionId, targetName);
            }
            /*
            // Don't add self to groups; we can notify self through other means
            //await Groups.AddToGroupAsync(Context.ConnectionId, curUserId.ToString());
            */
            await Groups.AddToGroupAsync(Context.ConnectionId, curUserId.ToString());
        }

        public override Task OnDisconnectedAsync(Exception? exception)
        {
            _logger.LogInformation("User disconnected: {0}", Context.User?.GetUserId());

            var curUserId = Context.User?.GetUserId();
            if (curUserId != null)
            {
                _connectionMapping.Remove(curUserId.Value, Context.ConnectionId);
            }
            return base.OnDisconnectedAsync(exception);
        }
    }
}
