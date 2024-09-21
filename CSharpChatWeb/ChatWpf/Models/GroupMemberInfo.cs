using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class GroupMemberInfo
    {
        public required int Id { get; set; }
        public required string UserName { get; set; }
        public bool IsOperator { get; set; }
    }
}
