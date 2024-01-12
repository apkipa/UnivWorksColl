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

// Returns hasMore
const onScrollLoad = async () => {
  console.log(client.collectionList())
  posts.push(...await client.collectionList())
  return false
}

let isLoading = false
const scrollHandler = async (event) => {
  let isBottom = false

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
    await onScrollLoad()
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
      break
    }
    await nextTick()
  }
})()

// onMounted(() => {
//   window.addEventListener("scroll", scrollHandler, false)
// })
// onUnmounted(() => {
//   window.removeEventListener("scroll", scrollHandler, false)
// })

</script>

<template>
  <main style="display: flex; flex-direction: column; box-sizing: border-box;">
    <div class="blur-background page-header-container">
      <div class="page-header">
        <div style="font-size: larger; font-weight: bold;">书签</div>
        <div style="font-size: small; color: gray; margin-top: 2px;">@{{ mainStore.userName }}</div>
      </div>
    </div>
    <PostItem class="post-item" v-for="i in posts" :key="i" :post="i"/>
    <div style="text-align: center; color: gray;">已经到底了</div>
  </main>
</template>

<style scoped>
@import '@/assets/styles.css';

.page-header-container {
  position: sticky;
  top: 0;
  padding: 4px 16px;
  z-index: 2;
}

.page-header {
  display: flex;
  flex-direction: column;
}

main > .post-item {
  border-bottom: 1px solid lightgray;
}

</style>
