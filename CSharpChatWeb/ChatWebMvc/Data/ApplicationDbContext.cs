using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Identity.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore;

namespace ChatWebMvc.Data
{
    public class ApplicationDbContext : IdentityDbContext<AppUser, IdentityRole<int>, int>
    {
        public ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
            : base(options)
        {
        }

        protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
        {
            base.OnConfiguring(optionsBuilder);
            optionsBuilder.AddInterceptors(new SoftDeleteInterceptor());
        }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<ChatMsg>()
                .HasQueryFilter(cm => !cm.IsDeleted);
            modelBuilder.Entity<ChatMsg>()
                .HasOne(cm => cm.Sender)
                .WithMany()
                .HasForeignKey(cm => cm.SenderId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<ChatMsg>()
                .HasOne(cm => cm.Receiver)
                .WithMany()
                .HasForeignKey(cm => cm.ReceiverId)
                .OnDelete(DeleteBehavior.NoAction);
            /*modelBuilder.Entity<ChatMsg>()
                .HasOne(cm => cm.SenderUserOrig)
                .WithMany()
                .HasForeignKey(cm => cm.SenderId)
                .IsRequired(false)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<ChatMsg>()
                .Ignore(cm => cm.SenderUser);*/

            modelBuilder.Entity<UserGroupJoin>()
                .HasOne(ugj => ugj.User)
                .WithMany(u => u.UserGroupJoins)
                .HasForeignKey(ugj => ugj.UserId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<UserGroupJoin>()
                .HasOne(ugj => ugj.Group)
                .WithMany(cg => cg.UserGroupJoins)
                .HasForeignKey(ugj => ugj.GroupId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<UserGroupJoin>()
                .HasOne(ugj => ugj.LastWatchedMsg)
                .WithMany()
                .HasForeignKey(ugj => ugj.LastWatchedMsgId)
                .IsRequired(false)
                .OnDelete(DeleteBehavior.NoAction);

            modelBuilder.Entity<UserFriendship>()
                .HasOne(uf => uf.From)
                .WithMany(u => u.Friendships)
                .HasForeignKey(uf => uf.FromId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<UserFriendship>()
                .HasOne(uf => uf.To)
                .WithMany()
                .HasForeignKey(uf => uf.ToId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<UserFriendship>()
                .HasOne(uf => uf.LastWatchedMsg)
                .WithMany()
                .HasForeignKey(uf => uf.LastWatchedMsgId)
                .IsRequired(false)
                .OnDelete(DeleteBehavior.NoAction);

            modelBuilder.Entity<FriendRequest>()
                .HasOne(fr => fr.From)
                .WithMany()
                .HasForeignKey(fr => fr.FromId)
                .OnDelete(DeleteBehavior.NoAction);
            modelBuilder.Entity<FriendRequest>()
                .HasOne(fr => fr.To)
                .WithMany()
                .HasForeignKey(fr => fr.ToId)
                .OnDelete(DeleteBehavior.NoAction);

            modelBuilder.Entity<ChatGroup>()
                .HasOne(cg => cg.Owner)
                .WithMany()
                .HasForeignKey(cg => cg.OwnerId)
                .OnDelete(DeleteBehavior.NoAction);
            /*modelBuilder.Entity<ChatGroup>()
                .HasOne(cg => cg.PrincipalNumber)
                .WithOne()
                .HasForeignKey<ChatGroup>(cg => cg.Id)
                .OnDelete(DeleteBehavior.NoAction);*/

            /*modelBuilder.Entity<AppUser>()
                .HasOne(u => u.PrincipalNumber)
                .WithOne()
                .HasForeignKey<AppUser>(u => u.Id)
                .OnDelete(DeleteBehavior.NoAction);*/

            /*modelBuilder.Entity<ChatGroup>()
                .HasKey(cg => cg.PrincipalNumberId);
            modelBuilder.Entity<ChatGroup>()
                .HasMany(cg => cg.UserGroupJoins)
                .WithOne(ugj => ugj.Group)
                .HasForeignKey(ugj => ugj.GroupId);

            modelBuilder.Entity<UserGroupJoin>()
                .HasKey(ugj => new { ugj.UserId, ugj.GroupId });
            modelBuilder.Entity<UserGroupJoin>()
                .HasOne(ugj => ugj.User)
                .WithMany(u => u.UserGroupJoins)
                .HasForeignKey(ugj => ugj.UserId);
            modelBuilder.Entity<UserGroupJoin>()
                .HasOne(ugj => ugj.Group)
                .WithMany(cg => cg.UserGroupJoins)
                .HasForeignKey(ugj => ugj.GroupId);

            modelBuilder.Entity<AppUser>()
                .HasMany(u => u.Friends)
                .WithMany()
                .UsingEntity(j => j.ToTable("Friendship"));

            modelBuilder.Entity<PrincipalNumber>()
                .HasKey(pn => pn.Id);

            modelBuilder.Entity<ChatMsg>()
                .HasKey(cm => cm.Id);
            modelBuilder.Entity<ChatMsg>()
                .HasOne(cm => cm.Sender)
                .WithMany()
                .HasForeignKey(cm => cm.SenderId);
            modelBuilder.Entity<ChatMsg>()
                .HasOne(cm => cm.Group)
                .WithMany()
                .HasForeignKey(cm => cm.GroupId);*/
        }

        public DbSet<ChatGroup> ChatGroups { get; set; } = null!;
        public DbSet<UserGroupJoin> UserGroupJoins { get; set; } = null!;
        public DbSet<PrincipalNumber> PrincipalNumbers { get; set; } = null!;
        public DbSet<ChatMsg> ChatMsgs { get; set; } = null!;
        public DbSet<UserFriendship> UserFriendships { get; set; } = null!;
        public DbSet<FriendRequest> FriendRequests { get; set; } = null!;
    }
}
