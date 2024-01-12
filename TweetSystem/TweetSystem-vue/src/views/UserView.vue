<script setup>
import { ref, reactive, computed, VueElement, nextTick, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import PostItem from '../components/PostItem.vue'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

const userName = ref(route.params.user_name);
const isSelfUser = ref(false);
const isFollowing = ref(false);
const isBlocking = ref(false);

const goBack = () => {
  router.back()
}

// var posts = reactive([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
// // Returns hasMore
// const onScrollLoad = async () => {
//   posts.push(1, 2, 3, 4)
//   return true
// }

var userInfo = ref()

var posts = reactive([])
// Returns hasMore
const onScrollLoad = async () => {
  posts.push(...await client.postList(userInfo.value.id))
  console.log(posts)
  // console.log(await client.postList(userInfo.value.id))
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
};

// Populate content
(async () => {
  try {
    userInfo.value = await client.userGetInfoByName(userName.value)
    isSelfUser.value = userInfo.value.id == mainStore.userId
  } catch (e) {
    console.log(e.message)
    if (e.message.includes('用户不存在')) {
      ElMessage.error('此用户不存在')
    }
  }
  // console.log(userInfo.value)

  // Load user relationship
  const userRelationshipData = await client.userRelationshipGetTwoRelation(mainStore.userId, userInfo.value.id);
  isFollowing.value = userRelationshipData.is_following;
  isBlocking.value = userRelationshipData.is_blocking;

  // Has vertical scroll bar?
  while (!(window.innerWidth > document.documentElement.clientWidth)) {
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

const onClickFollow = async () => {
  await client.userRelationshipFollow(userInfo.value.id);
  isFollowing.value = true;
  isBlocking.value = false;
};
const onClickUnfollow = async () => {
  await client.userRelationshipUnfollow(userInfo.value.id);
  isFollowing.value = false;
  isBlocking.value = false;
};
const onClickBlock = async () => {
  await client.userRelationshipBlock(userInfo.value.id);
  isFollowing.value = false;
  isBlocking.value = true;
};

</script>

<template>
  <div>
    <!-- Username is: {{ $route.params.user_name }} -->
    <el-page-header @back="goBack" class="blur-background page-header-container"
      title=" "
    >
      <template #content>
        <div class="page-header">
          <div style="font-size: 19.2px; font-weight: bold; margin-top: 2px; margin-bottom: -2px;">{{ userInfo?.nickname ?? userName }}</div>
          <div style="font-size: small; color: gray;">{{ posts.length }} 篇推文</div>
        </div>
      </template>
    </el-page-header>
    <div class="user-info-container" v-if="userInfo">
      <div class="user-info-top-photo-container">
      </div>
      <div class="user-info-top-line-container">
        <div class="user-img-container">
          <font-awesome-icon :icon="['fas', 'user']"/>
        </div>
        <div class="user-info-actions-area">
          <el-popover v-if="!isSelfUser" placement="left-start" :width="300" trigger="click">
            <template #reference>
              <el-button circle>
                <font-awesome-icon :icon="['fas', 'ellipsis']" />
              </el-button>
            </template>
            <el-button style="width: 100%; justify-content: left;" text @click="onClickBlock">拉黑 @{{ userName }}</el-button>
          </el-popover>
          <el-button v-if="!isSelfUser && (isFollowing || isBlocking)" @click="onClickUnfollow">取消{{ isBlocking ? '拉黑' : '关注' }}</el-button>
          <el-button v-if="!isSelfUser && (!isFollowing && !isBlocking)" @click="onClickFollow">关注</el-button>
          <el-button v-if="isSelfUser" @click="$router.push('/settings/profile')">编辑个人资料</el-button>
        </div>
      </div>
      <div class="user-name-container">
        <div style="font-size: x-large; font-weight: bold;">{{ userInfo.nickname }}</div>
        <div style="font-size: smaller; color: gray;">@{{ userName }}</div>
      </div>
      <div class="user-introduction-container">
        {{ userInfo?.introduction }}
      </div>
    </div>
    <div class="media-container" v-if="userInfo">
      <el-menu mode="horizontal" default-active="1" :ellipsis="false">
        <el-menu-item index="1">推文</el-menu-item>
      </el-menu>
      <PostItem class="post-item" v-for="i in posts" :key="i.id" :post="i"/>
      <div style="text-align: center; color: gray;">已经到底了</div>
    </div>
    <div class="failure-container" v-if="!userInfo">
      <div style="text-align: center; color: gray;">加载失败</div>
    </div>
  </div>
</template>

<style scoped>
@import '@/assets/styles.css';

.page-header-container {
  position: sticky;
  top: 0;
  padding: 2px 16px;
  z-index: 2;
}

.page-header {
  display: flex;
  flex-direction: column;
}

.user-info-top-photo-container {
  width: 100%;
  background-color: #cfd9de;
  height: 200px;
}

.user-img-container {
  display: flex;
  border-style: solid;
  border-width: 4px;
  border-radius: 1000px;
  border-color: white;
  background-color: bisque;
  margin-top: -64px;
  margin-left: 20px;
  font-size: 60px;
  align-items: center;
  justify-content: center;
  height: 120px;
  width: 120px;
}

.user-info-top-line-container {
  display: flex;
  flex-direction: row;
}

.user-info-actions-area {
  margin-left: auto;
  margin-right: 16px;
  margin-top: 10px;
}

.user-info-actions-area > button {
  border-radius: 1000px;
}

.user-name-container {
  margin-top: 16px;
  margin-left: 16px;
  display: flex;
  flex-direction: column;
}

.user-introduction-container {
  margin-top: 16px;
  margin-left: 16px;
}

.media-container {
  margin-top: 16px;
}

.media-container > .post-item {
  border-bottom: 1px solid lightgray;
}

</style>
