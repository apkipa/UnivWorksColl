<script setup>
import { ref, reactive, VueElement, onMounted, onUnmounted, computed, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import { useScroll } from '@vueuse/core'
import PostItem from '../components/PostItem.vue'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

var posts = reactive([])
var ps = 20
var pn = 0
// Returns hasMore
const onScrollLoad = async () => {
  var pagable = await client.postRecommendGenerate(pn, ps)
  posts.push(...pagable.content)
  pn++
  return true
}

let isLoading = false
let isAllLoaded = false
const scrollHandler = async (event) => {
  let isBottom = false

  if (isAllLoaded) {
    return
  }

  if ((window.innerHeight + window.scrollY) - document.body.offsetHeight >= -60) {
    isBottom = true
    console.log('bottom')
  }

  if (!isBottom) {
    return
  }

  // Reached bottom
  if (isLoading) { return; }
  isLoading = true
  try {
    if (!await onScrollLoad()) {
      isAllLoaded = true
    }
  } catch (e) {
    throw e
  } finally {
    isLoading = false
  }
}

// Populate content
(async () => {
  // Has vertical scroll bar?
  var cnt = 0
  while (!(window.innerWidth > document.documentElement.clientWidth)) {
    cnt += 1
    if (cnt > 10) {
      break
    }
    if (!await onScrollLoad()) {
      isAllLoaded = true
      break
    }
    await nextTick()
  }
})()

onMounted(() => {
  window.addEventListener("scroll", scrollHandler, false)
})
onUnmounted(() => {
  window.removeEventListener("scroll", scrollHandler, false)
})

</script>

<template>
  <main style="display: flex; flex-direction: column; box-sizing: border-box;">
    <!-- <div>User name is: {{ mainStore.userName }}</div> -->
    <!-- <TheWelcome /> -->
    <el-menu default-active="1" mode="horizontal" class="blur-background"
    :ellipsis="false" style="position: sticky; top: 0;"
    >
      <el-menu-item index="1">为你推荐的推文</el-menu-item>
    </el-menu>
    <!-- <div v-infinite-scroll="onScrollLoad" class="infinite-list"
      infinite-scroll-distance="10"
      style="height: auto; overflow: auto;"
    >
      <PostItem v-for="i in posts" :key="i" class="infinite-list-item" :post="i"/>
    </div> -->
    <PostItem class="post-item" v-for="i in posts" :key="i" :post="i"/>
    <!-- <div id="lipsum" style="padding: 0 4px;">
<p>
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam a ullamcorper nunc, auctor convallis quam. Vivamus sed quam eu nisl mollis tincidunt quis a turpis. Nam imperdiet massa quam, vitae eleifend magna cursus vel. Pellentesque risus dolor, imperdiet eu ullamcorper nec, elementum sed urna. Curabitur vel tempus erat. Nunc lobortis ultricies nisi, lacinia fringilla est. Morbi fermentum arcu eu lectus faucibus suscipit. Etiam efficitur mauris a pulvinar cursus. Vivamus eu lorem pellentesque, volutpat nisi eu, malesuada massa. Nullam rutrum tempor vehicula. Vestibulum maximus tristique erat id feugiat. Vivamus vitae tortor id ipsum pulvinar consequat.
</p>
<p>
Quisque mattis tempor arcu, et fermentum lorem rhoncus rutrum. Aliquam erat volutpat. Vivamus lacinia iaculis lectus vitae imperdiet. Donec posuere varius auctor. Aenean a interdum velit, ut volutpat mauris. Nam vel lectus ac enim facilisis mollis et a dolor. Aenean in rhoncus risus. Aliquam augue orci, sodales id blandit sit amet, rutrum facilisis lectus.
</p>
<p>
Phasellus eleifend mauris at ornare dictum. Nam ut vehicula tortor. Nunc porttitor id arcu vitae tempor. Morbi accumsan velit et hendrerit facilisis. In non dignissim tortor. In vulputate ipsum sollicitudin mi laoreet, quis malesuada ligula consequat. Mauris facilisis nisl non lectus dictum mollis. Sed mollis convallis elementum. Ut at nisi metus. Donec vitae consequat nisl. Maecenas et nisi sit amet dui mollis pulvinar. Cras fermentum pulvinar sem in elementum. Phasellus cursus ante nunc, a tristique justo commodo in. Nam feugiat bibendum velit in varius.
</p>
<p>
Sed eleifend efficitur erat sed mattis. Nam vulputate metus ex, at tempor justo ullamcorper vitae. Sed sit amet viverra erat, in tempus lorem. Nulla sit amet elit non mauris vulputate mollis nec quis leo. Nulla facilisi. Nulla venenatis neque id vestibulum condimentum. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos.
</p>
<p>
Ut pharetra dignissim quam, ut varius mi consequat eget. Proin consequat tortor nisi, at euismod felis gravida a. Mauris viverra pretium tristique. Duis tempus ligula eget massa tempor mattis. Nullam non metus vel ligula accumsan fermentum vel non sem. Mauris eu ex a lorem egestas blandit sed nec mauris. Fusce eget tortor et nunc laoreet rhoncus. Fusce urna enim, posuere quis suscipit eu, sagittis sit amet dui. Ut ante leo, tristique ut ante a, finibus tincidunt arcu.
</p>
<p>
Phasellus blandit eu nunc at maximus. Vivamus porta tristique maximus. Proin id velit faucibus odio vulputate congue. Donec in pulvinar nulla. Duis id magna tempor, volutpat augue quis, imperdiet lacus. Cras hendrerit egestas est, ut fringilla metus porta nec. Phasellus mollis ut purus quis malesuada. In vel odio aliquam, dapibus orci non, luctus purus. Nullam semper ipsum a lacus faucibus, posuere porttitor diam efficitur. Aliquam id erat enim. Suspendisse mauris sapien, hendrerit nec odio sed, aliquet pretium urna. Donec quis lobortis felis. Fusce pulvinar auctor tincidunt. Cras at scelerisque augue, vitae vulputate nisl. Donec tempor facilisis massa, eget rutrum diam iaculis sit amet. Morbi quam massa, varius aliquet nibh nec, faucibus consequat nulla.
</p>
<p>
Vivamus aliquet, est vitae hendrerit aliquam, lorem lacus laoreet leo, at pretium elit massa sit amet mauris. Proin a tempus nunc. Morbi tristique nulla sem, ut porttitor lacus auctor at. Phasellus pellentesque mauris lorem, non semper nulla ultrices ac. Suspendisse nec rhoncus nunc. Sed vel quam et nunc malesuada viverra vitae a lectus. Vivamus ac tortor at nisl commodo gravida et eget ex. Sed luctus leo quis justo elementum, nec auctor dolor sollicitudin. Nullam ac turpis et magna tempus pellentesque. Nulla vel neque a lectus accumsan pulvinar et vel nisi. Donec eu risus sapien.
</p>
<p>
Maecenas dignissim nibh et hendrerit dapibus. Nulla sapien elit, vulputate nec tortor non, tempus egestas nibh. Aliquam volutpat mi vitae mauris aliquam dignissim. Praesent erat tellus, sodales ut placerat eleifend, consequat vitae est. Ut id justo risus. Mauris lacinia tristique risus et dignissim. Vestibulum imperdiet leo vel lobortis maximus. Ut imperdiet cursus turpis sit amet scelerisque. Ut pretium neque vel tempus mollis. In rhoncus semper enim, sit amet auctor mauris maximus eget. Nam bibendum interdum orci, id laoreet lacus volutpat et. Donec pulvinar convallis sem, at sollicitudin risus pharetra non.
</p>
<p>
Pellentesque iaculis, ligula quis fringilla hendrerit, dui turpis pretium augue, id pulvinar elit mi non augue. Fusce malesuada nibh eu eleifend finibus. Quisque tempor consequat libero, eget tempus sapien cursus ut. Morbi ligula mi, sagittis vitae libero ac, euismod venenatis felis. Vestibulum a neque ipsum. Mauris tempor, mauris at mattis dictum, ante leo eleifend neque, volutpat volutpat lectus risus in sem. Praesent lorem libero, molestie sed varius quis, suscipit in est. Etiam eleifend sem nec ligula tempor, quis ullamcorper orci tincidunt. Nam at volutpat dui. Vestibulum nisi enim, fermentum sed malesuada ut, pretium a nisl. Sed pellentesque eros eu nunc lacinia posuere. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Donec tempus mollis sapien. In euismod interdum pretium. Integer ullamcorper sapien at rutrum semper. Nunc a egestas risus.
</p>
<p>
Duis consequat vulputate ex, vel congue elit suscipit dictum. In tincidunt nisl ac varius elementum. Etiam eget sollicitudin felis. Morbi faucibus libero ut mauris ullamcorper, et dapibus ipsum pellentesque. Maecenas eleifend lobortis quam a semper. Nullam ac purus felis. Duis tellus eros, interdum eu tempor eu, tincidunt at lacus. Integer rhoncus enim finibus egestas posuere. Proin blandit, velit a molestie tincidunt, lorem tortor ullamcorper arcu, vel suscipit arcu mi sed tellus. Curabitur consectetur tellus quis ante interdum consequat. Curabitur molestie, enim sed pellentesque ornare, diam tortor pharetra libero, ac accumsan massa massa eget nunc. Vestibulum vitae odio tellus. Ut non quam ac nulla maximus ornare.
</p>
<p>
Maecenas aliquam at orci id consequat. Praesent blandit, arcu pulvinar tincidunt ullamcorper, odio nisl tristique turpis, in volutpat massa nunc eget massa. Ut eget mi sed risus egestas lacinia. Nunc vestibulum est eu enim aliquam varius non a velit. Donec massa orci, dictum sed enim non, sollicitudin tempus ligula. Maecenas volutpat iaculis iaculis. In semper nec risus ac imperdiet. Pellentesque mattis semper nisi, nec aliquam odio eleifend sed. Nulla quis suscipit ex. Quisque tincidunt nibh urna, id euismod sem malesuada eget.
</p>
<p>
Donec rhoncus orci nulla, a cursus dui cursus nec. Quisque vestibulum, nulla sit amet sagittis molestie, eros arcu ultrices augue, et ultricies risus ante vel est. Etiam interdum nunc arcu. Donec aliquet suscipit gravida. Sed a blandit ex. Phasellus faucibus mauris nec arcu semper sollicitudin. Phasellus eu diam et dui dignissim tincidunt. Cras nunc justo, mollis vel nisl vel, mattis dapibus mauris. Fusce iaculis blandit viverra.
</p>
<p>
Cras vel leo quis lacus volutpat commodo porttitor id tellus. Donec gravida risus quis consectetur ultrices. Interdum et malesuada fames ac ante ipsum primis in faucibus. Phasellus sit amet felis vitae metus semper maximus. In luctus felis sit amet risus volutpat, ac maximus ipsum porta. Fusce cursus ex nec ultricies cursus. Maecenas condimentum et purus sed scelerisque. Nunc fermentum dapibus libero, sed finibus felis scelerisque sed. Maecenas quis finibus sapien. Morbi ultricies, dolor nec condimentum auctor, nibh arcu porta mauris, vel aliquam odio dolor id augue. Mauris suscipit sem vulputate leo efficitur, in consectetur dui viverra. Suspendisse dolor odio, dictum ut imperdiet eu, consequat a risus. Ut hendrerit elementum nibh at commodo. Suspendisse vel elit nec enim volutpat laoreet. Donec interdum odio nec urna aliquam pellentesque.
</p>
<p>
Maecenas varius justo orci, nec blandit tortor imperdiet quis. Cras faucibus auctor aliquam. In blandit sodales pharetra. Integer malesuada ex in mauris fermentum placerat. Donec porttitor, lorem id congue pellentesque, massa est sollicitudin magna, in aliquam mauris nunc et nisl. Fusce quis bibendum lectus. Praesent finibus leo eu pretium facilisis. In hac habitasse platea dictumst. Donec congue dolor ut placerat pretium. Maecenas vel magna non eros dignissim consequat sed in lorem. Curabitur tempor rhoncus sem quis dictum. Suspendisse urna justo, molestie quis nibh a, faucibus condimentum diam. Pellentesque tempor felis eu convallis pharetra.
</p>
<p>
Praesent finibus purus vitae tellus convallis, id luctus mi pulvinar. Integer pulvinar enim non orci vehicula, nec mattis erat imperdiet. Cras facilisis condimentum egestas. Morbi porttitor libero vitae efficitur porta. Fusce ullamcorper risus at luctus blandit. Aenean gravida diam eu ante gravida porttitor. Sed velit eros, vestibulum nec fringilla vel, cursus in dolor. Mauris tempus ex sed mattis elementum. Cras aliquam leo consectetur nulla mattis suscipit. Aliquam quis arcu id ipsum dignissim facilisis ut sed metus. Praesent faucibus enim odio, congue volutpat sapien iaculis vel. Aliquam maximus ullamcorper tortor eu consequat. Ut nulla ligula, semper ac molestie ut, porttitor nec nisi. Mauris a lorem libero.
</p>
<p>
Fusce ligula nisi, tristique posuere erat vel, lacinia eleifend urna. Sed accumsan blandit turpis in hendrerit. In hac habitasse platea dictumst. Praesent id ornare nisl, et pharetra purus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis condimentum aliquet felis, in lobortis nibh volutpat consectetur. Nunc luctus ac lorem sed pellentesque. Aliquam a nisi dui. Mauris consequat placerat nibh, ut congue arcu hendrerit at. Nunc vestibulum venenatis rhoncus.
</p>
<p>
Vestibulum ornare rutrum laoreet. Fusce accumsan, leo dictum pulvinar consequat, ipsum ipsum pretium tortor, at egestas lorem mi non tellus. Maecenas imperdiet eros in tortor placerat, et fringilla risus commodo. Cras eleifend quam in turpis cursus lacinia. Nam mollis quam in nunc ornare faucibus. Nunc orci mi, fermentum ut ipsum a, volutpat gravida erat. Fusce varius ac mi in congue. Sed non tristique urna. Duis cursus, metus sed scelerisque vestibulum, nunc neque placerat diam, non ornare lorem tellus ac massa. Phasellus in metus vel enim lobortis rutrum. Curabitur et augue cursus mauris condimentum dictum. Etiam vehicula ante ut sapien hendrerit tristique. Curabitur molestie eros ligula, ut feugiat nibh mattis non. Donec nec lacus auctor, faucibus libero in, commodo ipsum. Integer egestas sapien quis mi rutrum dignissim. Sed rutrum, augue sit amet tincidunt maximus, metus nisl imperdiet velit, at facilisis nibh est ut nibh.
</p>
<p>
Sed molestie magna vel enim fringilla, eget ultrices erat convallis. Ut at imperdiet lacus, ullamcorper porta metus. Maecenas vehicula aliquam pellentesque. Maecenas eget ligula hendrerit, vulputate ipsum viverra, congue turpis. Sed eu ligula eget diam dapibus sollicitudin. Suspendisse vestibulum dictum neque, sed lobortis felis hendrerit eget. Aenean eu nunc pretium, mollis mauris in, tincidunt nibh. Fusce eget enim ut massa maximus ornare. Duis consequat sagittis urna, vitae ultricies lacus ornare sit amet.
</p>
<p>
Quisque ac eros vestibulum, tempor massa ac, auctor mi. Pellentesque varius aliquet ligula at iaculis. Aenean sed aliquet ex. Nulla mattis quis mauris et eleifend. Cras eget vestibulum augue, id vehicula lorem. In hendrerit gravida felis, ac viverra dolor. Donec ultrices volutpat est, non facilisis metus. Integer est erat, scelerisque vel dolor id, aliquam pretium dolor. Donec sollicitudin sollicitudin blandit. Sed sodales, nisl quis fermentum vulputate, ligula dolor vehicula purus, vel lacinia dolor nunc sit amet orci. Sed rhoncus risus id dapibus finibus. Nullam id efficitur mi. Aenean semper dictum nunc. Sed egestas at lorem sit amet facilisis. Ut in sapien sit amet metus accumsan porta quis eget ex. Nunc a consectetur est, vel mattis nulla.
</p>
<p>
Donec sit amet diam faucibus, auctor odio eu, facilisis libero. Ut bibendum felis at tortor viverra, sed congue velit imperdiet. In ipsum mi, elementum eget feugiat nec, suscipit a ligula. In gravida, ante ac ullamcorper commodo, leo arcu dignissim lacus, in lacinia odio mauris in mauris. Quisque interdum malesuada dolor, ut vehicula diam tristique ut. Curabitur blandit est odio, ut placerat odio elementum sed. Nullam bibendum lectus et erat tincidunt iaculis sit amet nec ligula. Sed at sagittis lorem. Vivamus ac elit et justo semper dictum eu vulputate nisl. Vivamus sit amet nisi neque. Donec nec feugiat massa. Nulla pellentesque mi sed metus egestas vulputate.
</p></div> -->
    <div style="text-align: center; color: gray; margin-top: 4px;">已经到底了</div>
  </main>
</template>

<style scoped>
@import '@/assets/styles.css';

.infinite-list {
  height: 300px;
  padding: 0;
  margin: 0;
  list-style: none;
}
.infinite-list .infinite-list-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 50px;
  background: var(--el-color-primary-light-9);
  margin: 10px;
  color: var(--el-color-primary);
}
.infinite-list .infinite-list-item + .list-item {
  margin-top: 10px;
}

/* main > .post-item + .post-item {
  border-top: 1px solid lightgray;
} */

main > .post-item {
  border-bottom: 1px solid lightgray;
}

</style>
