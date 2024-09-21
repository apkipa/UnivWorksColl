using Microsoft.AspNetCore.Mvc.ModelBinding.Metadata;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Options;
using Microsoft.AspNetCore.Mvc.ModelBinding;

namespace ChatWebMvc.Misc
{
    public class KeepEmptyStringModelMetadataProvider : DefaultModelMetadataProvider
    {
        /// <inheritdoc />
        public KeepEmptyStringModelMetadataProvider(ICompositeMetadataDetailsProvider detailsProvider) : base(detailsProvider)
        {
        }

        /// <inheritdoc />
        public KeepEmptyStringModelMetadataProvider(ICompositeMetadataDetailsProvider detailsProvider, IOptions<MvcOptions> optionsAccessor) : base(detailsProvider, optionsAccessor)
        {
        }

        /// <inheritdoc />
        protected override ModelMetadata CreateModelMetadata(DefaultMetadataDetails entry)
        {
            return new DefaultModelMetadata(this, DetailsProvider, entry, ModelBindingMessageProvider)
            {
                DisplayMetadata =
            {
                ConvertEmptyStringToNull = false,
            },
            };
        }
    }
}
