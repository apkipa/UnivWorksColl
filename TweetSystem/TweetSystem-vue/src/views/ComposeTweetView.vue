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

const isSubmitting = ref(false);

const doBoolSwitchAsync = (b, f) => async () => {
  b.value = true
  try {
    return await f()
  } catch (e) {
    throw e
  } finally {
    b.value = false 
  }
};

const postForm = reactive({
  content: '',
});

const targetPost = ref(null);

const onCloseModal = async () => {
  router.back();
};

const onSubmit = doBoolSwitchAsync(isSubmitting, async () => {
  // await new Promise(resolve => setTimeout(resolve, 1000));
  if (route.query.forwardPost) {
    // Handle forward
    const newPostInfo = await client.postCreateForward(route.query.forwardPost);
    // NOTE: No need to commit
    ElMessage.success('推文已发布');
  }
  else {
    // Handle normal & reply
    var reply_post_id = null
    if (route.query.replyPost) {
      reply_post_id = route.query.replyPost;
    }
    const newPostInfo = await client.postCreate(postForm.content, reply_post_id);
    const resp = await client.postCommitAudit(newPostInfo.id);
    ElMessage.success('推文已发布，待审核');
  }

  // Final cleanup
  router.push(router.options.history.state.back);
});

// Load posts if needed
(async function() {
  const targetPostId = route.query.replyPost ?? route.query.forwardPost;
  if (!targetPostId) { return; }
  const targetPostData = await client.postView(targetPostId);
  targetPost.value = targetPostData;
})();

</script>

<template>
  <Teleport to="#modal-container">
    <div class="background-overlay">
      <div class="modal-container">
        <div class="post-box">
          <!-- <div id="post-box-header">登录到推文系统</div> -->
          <el-button circle text @click="onCloseModal">
            <el-icon size="20"><Close/></el-icon>
          </el-button>
          <!-- <el-divider /> -->
          <!-- <el-form id="login-box-form" :model="postForm" label-width="auto" label-position="left" @keyup.enter.native="onSubmitLogin">
            <el-form-item label="用户名">
              <el-input v-model="loginForm.username"/>
            </el-form-item>
            <el-form-item label="密码">
              <el-input v-model="loginForm.password" type="password"/>
            </el-form-item>
            <el-form-item>
              <el-button style="width: 48%;" type="primary" @click="onSubmitRegister" :loading="isRegistering">注册</el-button>
              <el-button style="width: 48%; margin-left: 4%;" @click="onSubmitLogin" :loading="isLogining">登录</el-button>
            </el-form-item>
          </el-form> -->
          <div v-if="$route.query.replyPost">正在回复 @{{ targetPost?.user?.nickname }} 的推文</div>
          <div v-if="$route.query.forwardPost">正在转发 @{{ targetPost?.user?.nickname }} 的推文</div>
          <PostItem v-if="targetPost" :post="targetPost" :has-actions="false" :is-embedded="true" :can-click-jump="false"/>
          <el-input v-model="postForm.content" type="textarea" class="input-area"
            rows="3" autosize
            placeholder="要发布些什么呢?"
            v-if="!$route.query.forwardPost"/>
          <el-button color="rgb(29, 155, 240)" style="margin-top: 12px; margin-left: auto;" :disabled="!$route.query.forwardPost && !postForm.content"
                      :loading="isSubmitting" @click="onSubmit">发推</el-button>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<style scoped>
.background-overlay {
  background: #00000080;
  width: 100%;
  height: 100%;
  pointer-events: auto;
}
.modal-container {
  position: absolute;
  margin-left: 32vw;
  margin-top: 10vh;
  /* padding-top: 20vh; */
  width: 36vw;
  height: auto;
  background: white;
  border-radius: 15px;
}

.post-box {
  display: flex;
  flex-direction: column;
  padding-left: 1em;
  padding-right: 1em;
  padding-top: 1em;
  padding-bottom: 0.8em;
  background-color: white;
  border-radius: 1em;
  /* box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19); */
}

.input-area {
  margin-top: 12px;
}

</style>
