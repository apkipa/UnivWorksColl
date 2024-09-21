using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using HandyControl.Tools.Extension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatWpf.ViewModel
{
    public class CreateNewGroupDialogVM : ObservableObject, IDialogResultable<string?>
    {
        public Action? CloseAction { get; set; }

        private string? _result = string.Empty;
        public string? Result
        {
            get => _result;
            set => SetProperty(ref _result, value);
        }

        public RelayCommand CloseCmd => new(() =>
        {
            Result = null;
            CloseAction?.Invoke();
        });
        public RelayCommand CreateCmd => new(() =>
        {
            CloseAction?.Invoke();
        });
    }
}
