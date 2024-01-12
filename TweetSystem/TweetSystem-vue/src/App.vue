<script setup>
// import { RouterLink, RouterView } from 'vue-router'
// import HelloWorld from './components/HelloWorld.vue'

import { ref, reactive, computed, VueElement, nextTick, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import { useScroll } from '@vueuse/core'
import { vScroll } from '@vueuse/components'
import { useCookies } from "vue3-cookies";

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()
const { cookies } = useCookies()

const thisUserPath = computed(() => '/user/' + mainStore.userName);

if (!cookies.isKey('satoken')) {
  router.push('/login');
} else {
  nextTick(async () => {
    try {
      await client.updateStore();
    } catch (e) {
      router.push('/login')
      if (!e.message.toLowerCase().includes('Token')) {
        ElMessage.error(`${e}`)
        ElMessage.error('请求失败，请重新登录')
      }
    }
  });
}

const onLogout = async () => {
  await client.authLogout();
  cookies.remove('satoken');
  try {
    await client.updateStore();
  } catch (e) {
    // Do nothing
  }
  router.push('/login');
}

const onComposeTweet = async () => {
  router.push({ name: 'compose-tweet', query:
    { replyPost: null, forwardPost: null },
  })
}

// let isLoading = false
// const scrollHandler = async (event) => {
//   // console.log(event)

//   // const scrollHeight = event.target.scrollHeight
//   // const scrollTop = event.target.scrollTop
//   // const clientHeight = event.target.clientHeight

//   let isBottom = false

//   if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight) {
//     isBottom = true
//   }

//   // if (scrollTop + clientHeight - scrollHeight < 100) {
//   //   return;
//   // }
//   if (!isBottom) {
//     return
//   }

//   // Reached bottom
//   if (isLoading) { return; }
//   isLoading = true
//   try {
//     // await onScrollLoad()
//   } catch (e) {
//     throw e
//   } finally {
//     isLoading = false
//   }
// }

// onMounted(() => {
//   window.addEventListener("scroll", scrollHandler, false)
// })
// onUnmounted(() => {
//   window.removeEventListener("scroll", scrollHandler, false)
// })

</script>

<template>
  <!-- <header>
    <img alt="Vue logo" class="logo" src="@/assets/logo.svg" width="125" height="125" />

    <div class="wrapper">
      <HelloWorld msg="You did it!" />

      <nav>
        <RouterLink to="/">Home</RouterLink>
        <RouterLink to="/about">About</RouterLink>
      </nav>
    </div>
  </header> -->

  <!-- <RouterView /> -->

  <div class="wrap">
    <div class="left">
      <!-- <RouterLink to="/home">Home</RouterLink>
      <RouterLink to="/login">Login</RouterLink>
      <RouterLink to="/about">About</RouterLink> -->
      <div class="sidebar-nav-header">推文系统</div>
      <el-menu :default-active="$router.currentRoute.value.path" :router="true"
        style="border-width: 0;"
      >
        <el-menu-item index="/home">
          <template #title>
            <el-icon><House/></el-icon>
            <span>主页</span>
          </template>
        </el-menu-item>
        <el-menu-item index="/search">
          <el-icon><Search/></el-icon>
          <span>搜索</span>
        </el-menu-item>
        <el-menu-item :index="thisUserPath">
          <el-icon><User/></el-icon>
          <span>个人资料</span>
        </el-menu-item>
        <el-menu-item index="/i/bookmarks">
          <font-awesome-icon :icon="['far', 'bookmark']" style="margin: 0 12px 0 6px;"/>
          <span>书签</span>
        </el-menu-item>
        <el-menu-item index="/settings">
          <el-icon><Setting/></el-icon>
          <span>设置</span>
        </el-menu-item>
        <el-menu-item index="/audit/tweet" v-if="mainStore.userRights?.includes('post.review')">
          <el-icon><stamp/></el-icon>
          <span>推文审核</span>
        </el-menu-item>
      </el-menu>
      <el-button color="rgb(29, 155, 240)"
        @click="onComposeTweet"
        style="border-radius: 1000px; font-size: large; font-weight: bold; height: 48px; margin: 12px 28px 0 12px;"
      >
        发推
      </el-button>
      <el-popover placement="top" :width="300" trigger="click">
        <template #reference>
          <div class="sidebar-nav-bottom">
            <div class="user-img-container">
              <el-icon :size="20"><Avatar/></el-icon>
            </div>
            <div style="margin: auto 0; font-size: 14px;">
              <span style="font-weight: bold;">{{ mainStore.nickName }}</span>
              <div style="color: gray;">@{{ mainStore.userName }}</div>
            </div>
          </div>
        </template>
        <el-button type="danger" @click="onLogout">退出登录</el-button>
      </el-popover>
    </div>
    
    <!-- <div class="middle">
      <RouterView :key="$route.path"/>
      <RouterView v-slot="{ Component }">
        <component :is="Component" :key="$route.fullPath"/>
      </RouterView>
      <RouterView name="ModalPopOut"/>
    </div> -->
    <div class="middle">
      <router-view v-slot="{ Component }">
        <stack-keep-alive v-slot='{ key }'> 
          <component :is="Component" :key='key'/>
        </stack-keep-alive>
      </router-view>
      <RouterView name="ModalPopOut"/>
    </div>
   
    <!-- <div class="middle">
      <RouterView v-slot="{ Component }">
        <VuePageStack>
          <component :is="Component" :key="$route.fullPath"/>
        </VuePageStack>
      </RouterView>
      <RouterView name="ModalPopOut"/>
    </div> -->
    <div class="right">Right</div>
  </div>
</template>

<style scoped>
/* header {
  line-height: 1.5;
  max-height: 100vh;
}

.logo {
  display: block;
  margin: 0 auto 2rem;
}

nav {
  width: 100%;
  font-size: 12px;
  text-align: center;
  margin-top: 2rem;
}

nav a.router-link-exact-active {
  color: var(--color-text);
}

nav a.router-link-exact-active:hover {
  background-color: transparent;
}

nav a {
  display: inline-block;
  padding: 0 1rem;
  border-left: 1px solid var(--color-border);
}

nav a:first-of-type {
  border: 0;
}

@media (min-width: 1024px) {
  header {
    display: flex;
    place-items: center;
    padding-right: calc(var(--section-gap) / 2);
  }

  .logo {
    margin: 0 2rem 0 0;
  }

  header .wrapper {
    display: flex;
    place-items: flex-start;
    flex-wrap: wrap;
  }

  nav {
    text-align: left;
    margin-left: -1rem;
    font-size: 1rem;

    padding: 1rem 0;
    margin-top: 1rem;
  }
} */

.sidebar-nav-header {
  text-align: center;
  font-size: larger;
  font-weight: bold;
  margin: 10px;
}
.sidebar-nav-bottom {
  border-radius: 100px;
  cursor: pointer;
  margin-top: auto;
  padding-left: 8px;
  margin-bottom: 8px;
  background-color: white;
  height: 56px;
  display: flex;
  flex-direction: row;
}
.sidebar-nav-bottom:hover {
  background-color: aliceblue;
}
.user-img-container {
  display: flex;
  align-items: center;
  justify-content: center;
  margin-top: 2.6%;
  height: 70%;
  aspect-ratio: 1 / 1;
  border: 1px solid gray;
  border-radius: 50%;
  margin-right: 8px;
}

.wrap {
  display: grid;
  grid-template-columns: 300px auto 300px;
}

.left, .right {
  display: flex;
  flex-direction: column;
  height: 100vh;
  position: -webkit-sticky;
  position: sticky;
  top: 0;
}

.left {
  /* background: coral; */
  /* padding: 4px; */
}

.right {
  background: lightblue;
  /* padding: 4px; */
  display: none;
}

.middle {
  /* background: #555; */
  border-style: solid;
  border-color: lightgray;
  border-width: 0 1px;
  /* padding: 0 4px; */
  width: 42vw;
}
</style>
