using ChatWebMvc.Models.Enums;

namespace ChatWebMvc.Models
{
    public class PrincipalNumber
    {
        public int Id { get; set; }
        public PrincipalKind Kind { get; set; } = PrincipalKind.User;
    }
}
