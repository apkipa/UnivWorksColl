<script setup>
import { ref, reactive, watch, computed, VueElement, nextTick, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'
import PostItem from '../components/PostItem.vue'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

const searchInput = ref('');
const postsList = reactive([]);

const searchIndex = ref('posts')

watch(searchInput, async (new_val, old_val) => {
  // Load new search results
  if (!new_val) {
    postsList.length = 0;
    return;
  }
  const searchResults = await client.postSearch(new_val);
  postsList.length = 0;
  postsList.push(...searchResults);
});

const handleTypeSelect = async (key) => {
  searchIndex.value = key;
};

</script>

<template>
  <div>
    <el-input v-model="searchInput" class="search-box" size="large" clearable
      placeholder="输入要搜寻的内容..."/>
    <el-menu class="horizontal-stretch-menu-items" :default-active="searchIndex"
      style="border-style: none;" default-active="1" mode="horizontal"
      :ellipsis="false" @select="handleTypeSelect"
    >
      <el-menu-item index="posts">
        推文
      </el-menu-item>
      <el-menu-item index="users">
        用户
      </el-menu-item>
    </el-menu>
    <div class="posts-list-container" v-if="searchIndex == 'posts'">
      <PostItem class="post-item" v-for="i in postsList" :key="i" :post="i"/>
    </div>
    <div class="users-list-container" v-if="searchIndex == 'users'">
      用户列表
    </div>
    <div style="text-align: center; color: gray; margin-top: 8px;">已经到底了</div>
  </div>
</template>

<style scoped>
.search-box {
  margin-top: 8px;
  margin-left: 4px;
  margin-bottom: 4px;
  width: calc(100% - 4px * 2);
}

.horizontal-stretch-menu-items > li {
  flex: 2;
}

div > .post-item {
  border-bottom: 1px solid lightgray;
}

</style>
