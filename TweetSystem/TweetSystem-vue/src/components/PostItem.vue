<script setup>
const props = defineProps({
  post: Object,
  hasActions: {
    type: Boolean,
    default: true,
    required: false,
  },
  isEmbedded: {
    type: Boolean,
    default: true,
    required: false,
  },
  noTopLinks: {
    type: Boolean,
    default: false,
    required: false,
  },
  preventDefaultActions: {
    type: Boolean,
    default: true,
    required: false,
  },
  linkLineUp: {
    type: Boolean,
    default: false,
    required: false,
  },
  linkLineDown: {
    type: Boolean,
    default: false,
    required: false,
  },
  canClickJump: {
    type: Boolean,
    default: true,
    required: false,
  },
})
const emit = defineEmits({
  clickReply: null,
  clickForward: null,
  clickLike: null,
  clickBookmark: null,
  postDeleted: null,
})

const formatTimestamp = (ts) => {
  return new Date(ts).toLocaleString()
}

import { ref, reactive, computed, VueElement, nextTick, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox, rowProps } from 'element-plus'
import { useRouter, useRoute } from 'vue-router'
import { getClient } from '@/client'
import { useMainStore } from '@/stores/main'

const router = useRouter()
const route = useRoute()
const client = getClient()
const mainStore = useMainStore()

const thisUserId = mainStore.userId;

const realPost = computed(() => {
  return props.post?.forwardPost || props.post;
});
const isLiked = computed(() => {
  return realPost.value.likes.some(v => v.user.id == thisUserId)
});
const isCollected = computed(() => {
  return realPost.value.collections.some(v => v.user.id == thisUserId)
});

const replyJumpRoute = computed(() => {
  const replyPost = props.post?.replyPost;
  if (!replyPost) { return ''; }
  return `/user/${replyPost.user.name}/status/${replyPost.id}`;
});
const userJumpRoute = computed(() => {
  return `/user/${realPost.value.user.name}`;
});

const onClickPostWhole = async () => {
  if (!props.canClickJump) { return; }
  if (window.getSelection().type === "Range") {
    // Don't jump when selecting text
    return;
  }
  router.push({ name: 'post-view', params: {
    user_name: realPost.value.user.name,
    status_id: realPost.value.id,
  } })
}
const onClickReply = async () => {
  router.push({ name: 'compose-tweet', query:
    { replyPost: realPost.value.id, forwardPost: null, },
  })
}
const onClickForward = async () => {
  router.push({ name: 'compose-tweet', query:
    { replyPost: null, forwardPost: realPost.value.id, },
  })
}
const onClickLike = async () => {
  if (!isLiked.value) {
    var result = await client.likeAdd(realPost.value.id)
    realPost.value.likes.push({ user: { id: thisUserId } })
  } else {
    var result = await client.likeRemove(realPost.value.id)
    realPost.value.likes = realPost.value.likes.filter(v => v.user.id != thisUserId)
  }
}
const onClickCollect = async () => {
  if (!isCollected.value) {
    var result = await client.collectionAdd(realPost.value.id)
    realPost.value.collections.push({ user: { id: thisUserId } })
  } else {
    var result = await client.collectionRemove(realPost.value.id)
    realPost.value.collections = realPost.value.collections.filter(v => v.user.id != thisUserId)
  }
}
const onDeletePost = async () => {
  ElMessageBox.confirm(
    '此操作会永久移除推文及其相关数据，并且不可逆。',
    '要删除此推文吗?',
    {
      confirmButtonText: '是，删除',
      cancelButtonText: '否',
      type: 'warning',
    }
  ).then(async () => {
    const delData = await client.postDelete(props.post.id);
    router.go();
  });
}

</script>

<template>
  <div class="root-container" @click="onClickPostWhole">
    <div v-if="!noTopLinks && post.forwardPost" style="margin-left: 40px; color: gray; font-weight: bold; font-size: 13px; margin-top: 6px;">
      <font-awesome-icon :icon="['fas', 'share']" style="margin-right: 11px;" />{{ post.user.nickname }} 已转推
    </div>
    <div v-if="!noTopLinks && post.replyPost" style="margin-left: 40px; color: gray; font-weight: bold; font-size: 13px; margin-top: 6px;">
      <font-awesome-icon :icon="['fas', 'comment']" style="margin-right: 11px;" />
      <router-link class="hint-jump-link" :to="replyJumpRoute" @click.stop>对 {{ post.replyPost.user.nickname }} 推文的回复</router-link>
    </div>
    <div class="elastic-post-top-padding" v-if="noTopLinks || (!post.forwardPost && !post.replyPost)" style="margin-top: 8px;"></div>
    <div class="post-outer-container">
      <router-link class="hint-jump-link" :to="userJumpRoute" @click.stop>
        <div class="user-img-container">
          <font-awesome-icon :icon="['fas', 'user']" />
        </div>
      </router-link>
      <div class="post-right-part">
        <div class="post-right-top">
          <div class="post-user-name">
            <span style="font-weight: bold;">{{ realPost.user.nickname }}</span>
            <span style="color: gray; margin-left: 4px;">@{{ realPost.user.name }}</span>
          </div>
          <div class="post-dot-separator">·</div>
          <div class="post-publish-date">
            {{ formatTimestamp(realPost.publishTime) }}
          </div>
        </div>
        <div class="post-content">
          <!-- Post {{ post }} -->
          {{ realPost.content }}
        </div>
        <div class="post-actions-bar" v-if="hasActions">
          <div style="grid-column: 1;">
            <el-button class="action-button" @click.stop="onClickReply">
              <font-awesome-icon :icon="['far', 'comment']" />
              <div style="margin-left: 4px;">{{ realPost.replyPosts.length || '回复' }}</div>
            </el-button>
          </div>
          <div style="grid-column: 2;">
            <el-button class="action-button" @click.stop="onClickForward">
              <font-awesome-icon :icon="['fas', 'share']" />
              <div style="margin-left: 4px;">转发</div>
            </el-button>
          </div>
          <div style="grid-column: 3;">
            <el-button class="action-button" @click.stop="onClickLike">
              <font-awesome-icon :icon="[isLiked ? 'fas' : 'far', 'heart']" />
              <div style="margin-left: 4px;">{{ realPost.likes.length || '喜欢' }}</div>
            </el-button>
          </div>
          <div style="grid-column: 4;">
            <el-button class="action-button" @click.stop="onClickCollect">
              <font-awesome-icon :icon="[isCollected ? 'fas' : 'far', 'bookmark']"/>
              <div style="margin-left: 4px;">收藏</div>
            </el-button>
          </div>
          <div style="grid-column: 5;" v-if="post.user.name == mainStore.userName">
            <el-button class="action-button" circle="true" type="danger" text @click.stop="onDeletePost"
                        style="margin-right: 4px;">
              <font-awesome-icon :icon="['far', 'trash-can']" />
            </el-button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.post-outer-container {
  display: flex;
  flex-direction: row;
  /* width: 100vw; */
  /* width: 100%; */
  width: 100%;
}

.user-img-container {
  grid-row: 1;
  grid-column: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  /* margin-top: 12px; */
  margin-top: 4px;
  margin-left: 12px;
  height: 40px;
  aspect-ratio: 1 / 1;
  border: 1px solid gray;
  border-radius: 50%;
  margin-right: 10px;
}

.post-right-part {
  grid-row: 2;
  grid-column: 2;
  /* margin-right: 12px; */
  width: 100%;
}

.post-dot-separator {
  color: gray;
  margin: 0 4px;
}

.post-right-top {
  display: flex;
  flex-direction: row;
  /* margin-top: 12px; */
  margin-top: 4px;
  font-size: 14px;
}

.post-user-name {
}

.post-publish-date {
  color: gray;
}

.post-content {
  margin-top: 2px;
  line-break: anywhere;
  width: 100%;
  white-space: pre-line;
}

.post-actions-bar {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr 1fr auto;
  margin-top: 6px;
  margin-bottom: 6px;
  /* width: 100%; */
}

.action-button {
  border-style: none;
}

.hint-jump-link {
  color: inherit;
  text-decoration: none;
}

</style>
