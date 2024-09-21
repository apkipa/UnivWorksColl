using ChatWpf.Misc;
using ChatWpf.Models.Enums;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using HandyControl.Tools.Extension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;

namespace ChatWpf.ViewModel
{
    public class ConfirmDialogVM : ObservableObject, IDialogResultable<ConfirmDialogResult>
    {
        public Action? CloseAction { get; set; }

        public ConfirmDialogResult Result { get; set; } = ConfirmDialogResult.Close;

        private string _title = string.Empty;
        public string Title
        {
            get => _title;
            set => SetProperty(ref _title, value);
        }

        private string _content = string.Empty;
        public string Content
        {
            get => _content;
            set => SetProperty(ref _content, value);
        }

        private string _iconKey = string.Empty;
        public string IconKey
        {
            get => _iconKey;
            set
            {
                SetProperty(ref _iconKey, value);
                IconGeometry = Utils.GetResource<Geometry>(value);
            }
        }

        private Geometry? _iconGeometry;
        public Geometry? IconGeometry
        {
            get => _iconGeometry;
            set => SetProperty(ref _iconGeometry, value);
        }

        private string _primaryStr = string.Empty;
        public string PrimaryStr
        {
            get => _primaryStr;
            set => SetProperty(ref _primaryStr, value);
        }

        private string _closeStr = string.Empty;
        public string CloseStr
        {
            get => _closeStr;
            set => SetProperty(ref _closeStr, value);
        }

        public RelayCommand PrimaryCommand => new(() =>
        {
            Result = ConfirmDialogResult.Primary;
            CloseAction?.Invoke();
        });

        public RelayCommand CloseCommand => new(() =>
        {
            Result = ConfirmDialogResult.Close;
            CloseAction?.Invoke();
        });
    }
}
