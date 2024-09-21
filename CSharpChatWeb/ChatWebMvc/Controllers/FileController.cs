using ChatWebMvc.Data;
using ChatWebMvc.Misc;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using System.Security.Cryptography;

namespace ChatWebMvc.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    [JsonAuthorize]
    [JsonExceptionFilter]
    public class FileController : ControllerBase
    {
        private readonly ApplicationDbContext _context;
        private readonly IWebHostEnvironment _env;

        public FileController(ApplicationDbContext context, IWebHostEnvironment env)
        {
            _context = context;
            _env = env;
        }

        [HttpPost("upload")]
        public async Task<IActionResult> Upload([FromForm] IFormFile file)
        {
            if (file == null)
            {
                return this.JsonErr(-400, "No file uploaded");
            }

            // Copy file to MemoryStream
            using var ms = new MemoryStream();
            await file.CopyToAsync(ms);
            ms.Position = 0;

            // Get SHA-256 hash of the file
            var sha256 = Convert.ToHexString(SHA256.HashData(ms));
            ms.Position = 0;

            var path = Path.Combine(_env.WebRootPath, "uploads", sha256);
            // If file already exists, do not store it again
            if (!System.IO.File.Exists(path))
            {
                using var stream = new FileStream(path, FileMode.Create);
                await file.CopyToAsync(stream);
            }

            return this.JsonOk(new { FileId = sha256 });
        }

        [HttpGet("download")]
        public async Task<IActionResult> Download([FromQuery] string fileId)
        {
            var path = Path.Combine(_env.WebRootPath, "uploads", fileId);
            if (!System.IO.File.Exists(path))
            {
                return this.JsonErr(-404, "File not found");
            }

            var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
            return File(stream, "application/octet-stream");
        }

        [HttpGet("preview")]
        public Task<IActionResult> Preview([FromQuery] string fileId)
        {
            // TODO: Maybe generate thumbnails for images
            return Download(fileId);
        }
    }
}
