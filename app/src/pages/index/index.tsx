import { View, Text, Image, Progress, Button, Switch, Input } from '@tarojs/components'
import React, { ChangeEvent, useEffect, useState } from 'react'
import Taro from '@tarojs/taro'
import './index.scss'

//传输状态
enum UploadStatus {
  uploading = '正在传输',
  success = '传输完成',
  fail = '传输失败',
  ready = '选择图片'
}

//循环模式
let looping = false
//图片数组
let arr_pic: string[] = []
//延时
let sleep_time: number = 0

const Index = () => {

  //上传进度
  const [progress, setProgress] = useState(0)
  //当前图片
  const [img, setImg] = useState('')
  //按钮蚊子
  const [btn, setBtn] = useState(UploadStatus.ready)
  //是否上传成功
  const [success, setSuccess] = useState(false)
  //选择的全部图片
  const [pics, setPics] = useState<string[]>([])
  //循环switch
  const [loop, setLoop] = useState<boolean>(false)
  //sleep
  const [sleep, setSleep] = useState<number>(0)

  //当选择的图片有变化时
  useEffect(() => {
    if (pics.length > 0) {
      arr_pic = pics
      starttask()
    }
  }, [pics])

  //当延时有变化时
  useEffect(() => {
    sleep_time = sleep
  }, [sleep])

  //当switch（是否重复） 有变化时
  useEffect(() => {
    looping = loop
    if (looping) {
      if (arr_pic.length > 0) {
        starttask()
      }
    }
  }, [loop])

  //被switch的值改变
  const on_change = (event: any) => {
    //设置是否循环
    setLoop(event.detail.value)
  }

  const on_input = (event: any) => {
    setSleep(event.detail.value)
  }

  //开始任务
  const starttask = () => {
    const max = arr_pic.length
    let index = 0
    setBtn(UploadStatus.uploading)
    upload(index, max)
  }

  //选择图片
  const selectImages = () => {
    Taro.chooseMessageFile({
      count: 100,
      success(res) {
        const files = res.tempFiles
        let arr: string[] = []
        files.map(file => {
          arr.push(file.path)
        })
        setPics(arr)
      }
    })
  }

  const upload = (index: number, max: number) => {
    //循环模式时，随时可以重新选择图片
    if (looping) {
      setBtn(UploadStatus.ready)
    }

    if (index >= max && !looping) {
      //非循环模式，只有最后一张发送完毕后，才可以重新选择图片
      setBtn(UploadStatus.ready)
      return false
    }

    //如果循环模式超过最后一张图，则变为第0张。
    if (index >= max && looping) {
      index = 0
    }

    setImg(arr_pic[index])
    setSuccess(false)
    const uploadTask = Taro.uploadFile({
      url: 'http://192.168.4.1/upload', //仅为示例，非真实的接口地址
      filePath: arr_pic[index],
      name: 'fileToUpload',
      timeout: 5000,
      header: {
        "Content-Type": "multipart/form-data; boundary="
      },
      success: function () {
        Taro.showToast({
          title: UploadStatus.success,
          icon: 'success',
          duration: 2000
        })
        setProgress(0)
        setSuccess(true)
        setTimeout(() => {
          upload(index + 1, max)
        }, sleep_time);
      },
      fail: function () {
        Taro.showToast({
          title: UploadStatus.fail,
          icon: 'none',
          duration: 2000
        })
        setProgress(0)
        upload(index, max)
      }
    })
    uploadTask.progress((res) => {
      setProgress(res.progress)
    })
  }

  return (
    <View className='page'>
      <Image
        className='img'
        src={img}
        style={
          {
            opacity: success ? 1 : 0.5
          }
        }
      />
      <Progress className='progress' percent={progress} strokeWidth={1} />
      <Text className='text'>135 * 240 JPG</Text>
      <View className='flex-wrp' style='flex-direction:row;'>
        <View className='flex-item'>循环：</View>
        <View className='flex-item'><Switch checked={loop} onChange={on_change} /></View>
      </View>

      {
        loop &&
        <View className='flex-wrp' style='flex-direction:row;'>
          <View className='flex-item'>延时：</View>
          <View className='flex-item'><Input value={sleep.toString()} type='number' onInput={on_input} placeholder='输入毫秒数' /></View>
        </View>
      }

      <Button
        disabled={
          UploadStatus.ready != btn
        }
        className='btn'
        onClick={selectImages}
        type='primary'>{btn}</Button>
    </View>
  )
}
export default Index