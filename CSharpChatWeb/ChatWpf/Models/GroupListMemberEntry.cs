using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.Models
{
    public class GroupListMemberEntry
    {
        public required int Id { get; set; }
        public required string UserName { get; set; }
        public required bool IsOperator { get; set; }

        public bool IsOwner { get; set; }
    }
}
