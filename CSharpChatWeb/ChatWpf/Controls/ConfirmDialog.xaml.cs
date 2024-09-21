using ChatWpf.Misc;
using ChatWpf.Models.Enums;
using ChatWpf.ViewModel;
using HandyControl.Controls;
using HandyControl.Tools.Extension;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ChatWpf.Controls
{
    /// <summary>
    /// Interaction logic for ConfirmDialog.xaml
    /// </summary>
    public partial class ConfirmDialog : Border
    {
        public static Task<ConfirmDialogResult> ShowAsync(string title, string content, string iconKey, string primaryStr, string CloseStr)
        {
            ConfirmDialogVM vmOuter = null!;
            var dialog = Dialog.Show<ConfirmDialog>("MainWndDialog");
            var task = dialog
                .Initialize<ConfirmDialogVM>((vm) =>
                {
                    vm.Title = title;
                    vm.Content = content;
                    vm.IconKey = iconKey;
                    vm.PrimaryStr = primaryStr;
                    vm.CloseStr = CloseStr;

                    vmOuter = vm;
                })
                .GetResultAsync<ConfirmDialogResult>();
            /*vmOuter.CloseAction = () =>
            {
                //HandyControl.Interactivity.ControlCommands.Close.Execute(null, (IInputElement)dialog);
                dialog.Close();
            };*/
            return task;
        }
        /*public static async Task<ConfirmDialogResult> ShowAsync(string title, string content, string iconKey, string primaryStr, string CloseStr)
        {
            var dialog = Dialog.Show<ConfirmDialog>("MainWndDialog");

            dialog
                .Initialize<ConfirmDialogVM>((vm) =>
                {
                    vm.Title = title;
                    vm.Content = content;
                    vm.IconKey = iconKey;
                    vm.PrimaryStr = primaryStr;
                    vm.CloseStr = CloseStr;
                })
                .Show();

            await Task.Delay(1000);

            HandyControl.Interactivity.ControlCommands.Close.Execute(null, (IInputElement)dialog);

            // get active window
            var activeWindow = Application.Current.Windows.OfType<System.Windows.Window>().SingleOrDefault(x => x.IsActive);
            var dec = Utils.GetDescendant<AdornerDecorator>(dialog);
            if (dec != null)
            {
                if (dec.Child != null)
                {
                    dec.Child.IsEnabled = true;
                }

                //dec.AdornerLayer?.Remove
            }

            return ConfirmDialogResult.Close;
        }*/

        public ConfirmDialog()
        {
            InitializeComponent();
        }
    }
}
