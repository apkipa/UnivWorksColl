using ChatWpf.Misc;
using Flurl.Http;
using System;
using System.Collections.Generic;
using System.Linq;
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
    /// Interaction logic for ImageEx.xaml
    /// </summary>
    public partial class ImageEx : Control
    {
        /*public int ImageWidth
        {
            get { return (int)GetValue(ImageWidthProperty); }
            set { SetValue(ImageWidthProperty, value); }
        }
        public static readonly DependencyProperty ImageWidthProperty = DependencyProperty.Register(
            nameof(ImageWidth), typeof(int), typeof(ImageEx), new PropertyMetadata(0));*/

        public Stretch Stretch
        {
            get { return (Stretch)GetValue(StretchProperty); }
            set { SetValue(StretchProperty, value); }
        }
        public static readonly DependencyProperty StretchProperty = DependencyProperty.Register(
            nameof(Stretch), typeof(Stretch), typeof(ImageEx), new PropertyMetadata(Stretch.Uniform));

        public Uri UriSource
        {
            get { return (Uri)GetValue(UriSourceProperty); }
            set { SetValue(UriSourceProperty, value); }
        }
        public static readonly DependencyProperty UriSourceProperty = DependencyProperty.Register(
            nameof(UriSource), typeof(Uri), typeof(ImageEx), new PropertyMetadata(null, UriSourcePropertyChanged));

        public CornerRadius CornerRadius
        {
            get { return (CornerRadius)GetValue(CornerRadiusProperty); }
            set { SetValue(CornerRadiusProperty, value); }
        }
        public static readonly DependencyProperty CornerRadiusProperty = DependencyProperty.Register(
            nameof(CornerRadius), typeof(CornerRadius), typeof(ImageEx), new PropertyMetadata(new CornerRadius(0)));

        public ImageEx()
        {
            InitializeComponent();
        }

        private static async void UriSourcePropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is ImageEx imageEx && e.NewValue is Uri uri)
            {
                var img = new BitmapImage();
                img.BeginInit();
                var resp = await Utils.HClient.Request(uri).WithCookies(Utils.HCookies).GetAsync();
                var stm = await resp.ResponseMessage.Content.ReadAsStreamAsync();
                img.StreamSource = stm;
                img.CacheOption = BitmapCacheOption.OnLoad;
                img.EndInit();
                img.Freeze();

                var innerImage = imageEx.GetTemplateChild("InnerImage") as Image;
                innerImage!.Source = img;
            }
        }
    }
}
