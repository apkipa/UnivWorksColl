using ChatWpf.Models.Enums;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class PrincipalNumber
    {
        public required int Id { get; set; }
        public required PrincipalKind Kind { get; set; }
    }
}
