using ChatWebMvc.Misc;
using ChatWebMvc.Models;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;
using static ChatWebMvc.Misc.ControllerExtensions;

// For more information on enabling Web API for empty projects, visit https://go.microsoft.com/fwlink/?LinkID=397860

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonExceptionFilter]
    public class AuthController : ControllerBase
    {
        private readonly SignInManager<AppUser> _signInManager;
        private readonly UserManager<AppUser> _userManager;

        public AuthController(SignInManager<AppUser> signInManager, UserManager<AppUser> userManager)
        {
            _signInManager = signInManager;
            _userManager = userManager;
        }

        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] (string username, string password) form)
        {
            var result = await _signInManager.PasswordSignInAsync(form.username, form.password, false, false);
            if (!result.Succeeded)
            {
                return this.JsonErr(-300, "登录失败");
            }
            return this.JsonOk(null);
        }

        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] (string username, string password) form)
        {
            var user = new AppUser
            {
                UserName = form.username,
                PrincipalNumber = new PrincipalNumber { Kind = Models.Enums.PrincipalKind.User },
            };
            var result = await _userManager.CreateAsync(user, form.password);
            if (!result.Succeeded)
            {
                return this.JsonErr(-300, "注册失败");
            }
            return this.JsonOk("注册成功，请等待管理员审批通过");
        }

        [JsonAuthorize]
        [HttpPost("logout")]
        public async Task<IActionResult> Logout()
        {
            await _signInManager.SignOutAsync();
            return this.JsonOk(null);
        }

        [JsonAuthorize]
        [HttpGet("myinfo")]
        public async Task<IActionResult> GetMyInfo()
        {
            var user = await _userManager.GetUserAsync(User) ?? throw new Exception("用户不存在");
            return this.JsonOk(new
            {
                user.UserName,
                user.Id,
                user.Email,
            });
        }

        // GET: api/<AuthController>
        [HttpGet]
        public IEnumerable<string> Get()
        {
            return new string[] { "value1", "value2" };
        }

        // GET api/<AuthController>/5
        [HttpGet("{id}")]
        public string Get(int id)
        {
            return "value";
        }

        // POST api/<AuthController>
        [HttpPost]
        public void Post([FromBody] string value)
        {
        }

        // PUT api/<AuthController>/5
        [HttpPut("{id}")]
        public void Put(int id, [FromBody] string value)
        {
        }

        // DELETE api/<AuthController>/5
        [HttpDelete("{id}")]
        public void Delete(int id)
        {
        }
    }
}
