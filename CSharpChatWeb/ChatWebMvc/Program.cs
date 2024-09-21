using ChatWebMvc.Data;
using ChatWebMvc.Hubs;
using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Filters;
using Microsoft.AspNetCore.Mvc.ModelBinding;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);

// Set to listen on port 5291
//builder.WebHost.UseUrls("http://localhost:5291");

builder.WebHost.UseStaticWebAssets();

// Add services to the container.
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection") ?? throw new InvalidOperationException("Connection string 'DefaultConnection' not found.");
builder.Services.AddDbContext<ApplicationDbContext>(options =>
{
    options.UseSqlServer(connectionString).EnableSensitiveDataLogging();
});
builder.Services.AddDatabaseDeveloperPageExceptionFilter();

builder.Services.AddDefaultIdentity<AppUser>(options => options.SignIn.RequireConfirmedAccount = true)
    .AddRoles<IdentityRole<int>>()
    .AddEntityFrameworkStores<ApplicationDbContext>();
builder.Services.AddControllersWithViews();
builder.Services.AddControllers(options =>
{
    //options.Filters.Add<JsonExceptionFilter>();
    options.ModelBinderProviders.Insert(0, new TupleModelBinderProvider());
});

builder.Services.AddSignalR();
builder.Services.AddRazorPages(options =>
{
    options.Conventions.AuthorizeFolder("/Admin", "Admin");
});
builder.Services.AddAuthorization(options =>
{
    options.AddPolicy("Admin", policy => policy.RequireRole("Admin"));
    options.AddPolicy("User", policy => policy.RequireRole("User"));
});

builder.Services.Configure<IdentityOptions>(options =>
{
    // Password settings.
    options.Password.RequireDigit = false;
    options.Password.RequireLowercase = false;
    options.Password.RequireNonAlphanumeric = false;
    options.Password.RequireUppercase = false;
    options.Password.RequiredLength = 2;
    options.Password.RequiredUniqueChars = 1;

    // Lockout settings.
    options.Lockout.DefaultLockoutTimeSpan = TimeSpan.FromMinutes(5);
    options.Lockout.MaxFailedAccessAttempts = 5;
    options.Lockout.AllowedForNewUsers = true;

    // User settings.
    options.User.AllowedUserNameCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._@+";
    options.User.RequireUniqueEmail = false;
});

builder.Services.AddSingleton<IModelMetadataProvider, KeepEmptyStringModelMetadataProvider>();
builder.Services.AddSingleton<IAuthorizationMiddlewareResultHandler, MyAuthorizationMiddlewareResultHandler>();
builder.Services.AddSingleton<IConnectionMapping<int>, ConnectionMapping<int>>();

var app = builder.Build();

// Seed the database
using (var scope = app.Services.CreateScope())
{
    var roleManager = scope.ServiceProvider.GetRequiredService<RoleManager<IdentityRole<int>>>();

    var addRoleIfNotExists = new Func<string, Task>(async roleName =>
    {
        if (!await roleManager.RoleExistsAsync(roleName))
        {
            await roleManager.CreateAsync(new IdentityRole<int>(roleName));
        }
    });

    await addRoleIfNotExists("Admin");
    await addRoleIfNotExists("User");

    var userManager = scope.ServiceProvider.GetRequiredService<UserManager<AppUser>>();
    var admin = await userManager.FindByNameAsync("admin");
    if (admin == null)
    {
        admin = new AppUser
        {
            UserName = "admin",
            Nickname = "管理员",
            PrincipalNumber = new PrincipalNumber { Kind = ChatWebMvc.Models.Enums.PrincipalKind.User },
            EmailConfirmed = true,
        };
        await userManager.CreateAsync(admin, "admin");
        await userManager.AddToRoleAsync(admin, "Admin");
        await userManager.AddToRoleAsync(admin, "User");
    }

    var createNormalUserIfNotExists = new Func<string, string, Task>(async (userName, nickname) =>
    {
        var user = await userManager.FindByNameAsync(userName);
        if (user == null)
        {
            user = new AppUser
            {
                UserName = userName,
                Nickname = nickname,
                PrincipalNumber = new PrincipalNumber { Kind = ChatWebMvc.Models.Enums.PrincipalKind.User },
                EmailConfirmed = true,
            };
            await userManager.CreateAsync(user, userName);
            await userManager.AddToRoleAsync(user, "User");
        }
    });

    for (int i = 1; i <= 10; i++)
    {
        await createNormalUserIfNotExists($"user{i}", $"普通用户{i}");
    }
}


// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.UseMigrationsEndPoint();
}
else
{
    app.UseExceptionHandler("/Home/Error");
}
app.UseStaticFiles();

app.UseRouting();
//app.UsePathBase("/api");

app.UseAuthentication();
app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");
app.MapRazorPages();
app.MapHub<ChatHub>("/api/hubs/chatHub");

app.Run();
