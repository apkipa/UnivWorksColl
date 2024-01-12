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

// console.log(route.params)

const userName = ref('Username');

const goBack = () => {
  router.back();
};

const onBlockUser = async () => {
  // TODO...
};

// var replyPosts = reactive([]);
const mainPost = ref(null);
const replyPost = ref(null);

(async function() {
  const mainPostId = route.params.status_id;
  const mainPostData = await client.postView(mainPostId);
  console.log(mainPostData)
  mainPost.value = mainPostData;
})();

</script>

<template>
  <div>
    <!-- Username is: {{ $route.params.user_name }} -->
    <el-page-header @back="goBack" class="blur-background page-header-container"
      title=" "
    >
      <template #content>
        <div class="page-header" style="font-size: larger; font-weight: bold;">
          推文
        </div>
      </template>
    </el-page-header>
    <div v-if="mainPost">
      <PostItem class="main-post-item" :post="mainPost" :can-click-jump="false"/>
      <div class="media-container">
        <el-menu mode="horizontal" default-active="1" :ellipsis="false">
          <el-menu-item index="1">回复</el-menu-item>
        </el-menu>
        <PostItem class="post-item" v-for="i in mainPost.replyPosts" :key="i" :post="i" :no-top-links="true"/>
        <div v-if="!mainPost.replyPosts.length">
          <div style="text-align: center; margin-top: 8px; color: gray;">暂无回复</div>
        </div>
      </div>
    </div>
    <div v-if="!mainPost">
      <div style="text-align: center;">推文加载失败</div>
    </div>
  </div>
</template>

<style scoped>
@import '@/assets/styles.css';

.page-header-container {
  display: flex;
  position: sticky;
  top: 0;
  padding: 2px 16px;
  z-index: 2;
  height: 48px;
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
  /* margin-top: 16px; */
}

div > .main-post-item {
  border-bottom: 1px solid lightgray;
}

.post-item {
  border-bottom: 1px solid lightgray;
}

</style>
